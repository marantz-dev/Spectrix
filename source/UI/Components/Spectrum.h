#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include "FFTProcessor.h"
#include "juce_graphics/juce_graphics.h"
#include "PluginParameters.h"
#include "UIutils.h"

template <int FFTSize> class SpectrumDisplay : public juce::Component, private juce::Timer {
  public:
    SpectrumDisplay(FFTProcessor<FFTSize> &spectralProcessor, const double sampleRateHz,
                    const juce::Colour spectrumColour, bool isDry = false)
        : processor(spectralProcessor), spectrumColour(spectrumColour), sampleRate(sampleRateHz),
          isDry(isDry) {
        magnitudes.resize(processor.getProcessedMagnitudes().size(), -100.0f);
        points.reserve(magnitudes.size() + 1); // Pre-allocate once
        startTimerHz(Parameters::FPS);
    }

    void paint(juce::Graphics &g) override {
        const auto &newMagnitudes
         = isDry ? processor.getUnprocessedMagnitudes() : processor.getProcessedMagnitudes();
        if(newMagnitudes.empty())
            return;

        updateMagnitudes(newMagnitudes);

        // Ensure binToX is valid
        if(binToX.size() != magnitudes.size()) {
            rebuildBinToX();
            if(binToX.empty())
                return; // Safety check
        }

        const auto bounds = getLocalBounds();
        const auto boundsBottom = static_cast<float>(bounds.getBottom());
        const auto boundsY = static_cast<float>(bounds.getY());
        const auto boundsX = static_cast<float>(bounds.getX());

        juce::Path spectrumPath;
        points.clear();
        points.emplace_back(boundsX, boundsBottom);

        // Build points using pre-calculated binToX
        for(size_t i = 1; i < magnitudes.size(); ++i) {
            const float x = binToX[i];
            float magnitudeDB = juce::jlimit(Parameters::minDBVisualizer,
                                             Parameters::maxDBVisualizer, magnitudes[i]);
            const float warpedDB = DBWarp(magnitudeDB);
            const float y = juce::jmap(warpedDB, Parameters::minDBVisualizer,
                                       Parameters::maxDBVisualizer, boundsBottom, boundsY);
            points.emplace_back(x, y);
        }

        spectralSmoothing(points);
        spectrumPath.startNewSubPath(points[0]);

        // Create smooth curve
        for(size_t i = 1; i < points.size(); ++i) {
            if(i == 1 || i == points.size() - 1) {
                spectrumPath.lineTo(points[i]);
            } else {
                const auto midPoint = juce::Point<float>((points[i - 1].x + points[i].x) * 0.5f,
                                                         (points[i - 1].y + points[i].y) * 0.5f);
                spectrumPath.quadraticTo(points[i - 1], midPoint);
            }
        }

        // Draw stroke
        g.setColour(spectrumColour.brighter());
        g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));

        // Complete path and fill with gradient
        spectrumPath.lineTo(points.back().x, boundsBottom);
        spectrumPath.lineTo(boundsX, boundsBottom);
        spectrumPath.closeSubPath();

        juce::ColourGradient gradient(spectrumColour.withAlpha(0.6f), boundsX, boundsY,
                                      spectrumColour.withAlpha(0.0f), boundsX, boundsBottom, false);

        g.setGradientFill(gradient);
        g.fillPath(spectrumPath);
    }

    void setSampleRate(double newSampleRate) {
        sampleRate = newSampleRate;
        rebuildBinToX();
    }

    void resized() override { rebuildBinToX(); }

    void visibilityChanged() override {
        if(isVisible()) {
            startTimerHz(Parameters::FPS);
        } else {
            stopTimer();
        }
    }

  protected:
    void timerCallback() override { repaint(); }

    void rebuildBinToX() {
        const auto b = getLocalBounds().toFloat();
        if(b.getWidth() <= 0.0f || sampleRate <= 0.0) {
            binToX.clear();
            return;
        }

        const float nyquistFreq = static_cast<float>(sampleRate) * 0.5f;
        const float minFreq = 20.0f;
        const float logMin = std::log10(minFreq);
        const float logMax = std::log10(nyquistFreq);
        const float logRange = logMax - logMin;
        const size_t n = magnitudes.size();
        const float nyquistOverN = nyquistFreq / (n > 1 ? (n - 1) : 1);
        const float boundsLeft = b.getX();
        const float boundsWidth = b.getWidth();

        binToX.resize(n);

        for(size_t i = 0; i < n; ++i) {
            const float frequency = i * nyquistOverN;
            const float logFreq = std::log10(juce::jmax(minFreq, frequency));
            binToX[i] = boundsLeft + ((logFreq - logMin) / logRange) * boundsWidth;
        }
    }

    void spectralSmoothing(std::vector<juce::Point<float>> &smoothPoints) {
        if(smoothPoints.size() < 3)
            return;

        constexpr float smoothingCoeff = 0.5f;
        constexpr float oneMinusSmoothingCoeff = 1.0f - smoothingCoeff;

        // Lambda for the smoothing operation
        auto applySmoothing = [](float &state, float input, float coeff) {
            state = coeff * input + (1.0f - coeff) * state;
        };

        // Forward pass
        float state = smoothPoints[0].y;
        for(size_t i = 1; i < smoothPoints.size(); ++i) {
            const float coeff = (smoothPoints[i].y > state) ? smoothingCoeff : 1.0f;
            applySmoothing(state, smoothPoints[i].y, coeff);
            smoothPoints[i].y = state;
        }

        // Backward pass
        state = smoothPoints.back().y;
        for(auto i = static_cast<int>(smoothPoints.size()) - 2; i >= 0; --i) {
            const float coeff = (smoothPoints[i].y > state) ? smoothingCoeff : 1.0f;
            applySmoothing(state, smoothPoints[i].y, coeff);
            smoothPoints[i].y = state;
        }
    }

    void updateMagnitudes(const auto &newMagnitudes) {
        constexpr float attackCoeff = 0.5f;
        constexpr float releaseCoeff = 0.05f;
        constexpr float oneMinusAttackCoeff = 1.0f - attackCoeff;
        constexpr float oneMinusReleaseCoeff = 1.0f - releaseCoeff;
        constexpr float minMagnitude = 1e-10f;
        constexpr float minDB = -100.0f;

        for(size_t i = 0; i < newMagnitudes.size() && i < magnitudes.size(); ++i) {
            const float magnitudeDB
             = newMagnitudes[i] > minMagnitude ? 20.0f * std::log10(newMagnitudes[i]) : minDB;

            if(magnitudeDB > magnitudes[i]) {
                magnitudes[i] = magnitudes[i] * oneMinusAttackCoeff + magnitudeDB * attackCoeff;
            } else {
                magnitudes[i] = magnitudes[i] * oneMinusReleaseCoeff + magnitudeDB * releaseCoeff;
            }
        }
    }

    FFTProcessor<FFTSize> &processor;
    std::vector<float> magnitudes;
    std::vector<juce::Point<float>> points; // Reused to avoid allocations
    std::vector<float> binToX;
    double sampleRate;
    bool isDry;
    juce::Colour spectrumColour;
};
