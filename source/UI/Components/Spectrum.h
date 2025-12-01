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
        : processor(spectralProcessor), spectrumColour(spectrumColour), sampleRate(sampleRateHz), isDry(isDry) {
        magnitudes.resize(processor.getProcessedMagnitudes().size(), -100.0f);
        startTimerHz(Parameters::FPS);
    }

    void paint(juce::Graphics &g) override {
        const auto &newMagnitudes = isDry ? processor.getUnprocessedMagnitudes()
                                          : processor.getProcessedMagnitudes();
        if(newMagnitudes.empty())
            return;

        updateMagnitudes(newMagnitudes);

        auto bounds = getLocalBounds();
        juce::Path spectrumPath;
        std::vector<juce::Point<float>> points;
        points.reserve(magnitudes.size());
        points.emplace_back(bounds.getX(), bounds.getBottom());
        if (binToX.size() != magnitudes.size()) {
            rebuildBinToX();
        }

        for(size_t i = 1; i < magnitudes.size(); ++i) {
            float x;
            if (i < binToX.size()) {
                x = binToX[i];
            } else {
                const float nyquistFreq = static_cast<float>(sampleRate) / 2.0f;
                const float minFreq = 20.0f;
                const float logMin = std::log10(minFreq);
                const float logMax = std::log10(nyquistFreq);
                float frequency = (i * nyquistFreq) / (magnitudes.size() - 1);
                float logFreq = std::log10(juce::jmax(minFreq, frequency));
                x = juce::jmap<float>(logFreq, (float)logMin, (float)logMax, bounds.getX(),
                                        bounds.getRight());
            }
            float magnitudeDB = magnitudes[i]; // Already in dB from updateMagnitudes
            magnitudeDB
             = juce::jlimit(Parameters::minDBVisualizer, Parameters::maxDBVisualizer, magnitudeDB);
            float warpedDB = DBWarp(magnitudeDB);
            float y = juce::jmap(warpedDB, Parameters::minDBVisualizer, Parameters::maxDBVisualizer,
                                 static_cast<float>(bounds.getBottom()),
                                 static_cast<float>(bounds.getY()));
            points.emplace_back(x, y);
        }

        spectralSmoothing(points);
        spectrumPath.startNewSubPath(points[0]);

        for(size_t i = 1; i < points.size(); ++i) {
            if(i == 1 || i == points.size() - 1) {
                spectrumPath.lineTo(points[i]);
            } else {
                auto midPoint = juce::Point<float>((points[i - 1].x + points[i].x) * 0.5f,
                                                   (points[i - 1].y + points[i].y) * 0.5f);
                spectrumPath.quadraticTo(points[i - 1], midPoint);
            }
        }

        g.setColour(spectrumColour.brighter());

        g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));

        spectrumPath.lineTo(points.back().x, bounds.getBottom());
        spectrumPath.lineTo(points[0].x, bounds.getBottom());
        spectrumPath.closeSubPath();

        juce::ColourGradient gradient(spectrumColour.withAlpha(0.6f), bounds.getX(), bounds.getY(),
                                      spectrumColour.withAlpha(0.0f), bounds.getX(),
                                      bounds.getBottom(), false);

        g.setGradientFill(gradient);
        g.fillPath(spectrumPath);

        juce::Path strokePath;
        strokePath.startNewSubPath(points[0]);
        for(size_t i = 1; i < points.size(); ++i) {
            strokePath.lineTo(points[i]);
        }

        g.setColour(spectrumColour);
    }

    void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; rebuildBinToX(); }
    void resized() override { rebuildBinToX(); }
    void visibilityChanged() override { }

  protected:
    void timerCallback() override { if (isShowing()) repaint(); }

    void rebuildBinToX() {
        auto b = getLocalBounds().toFloat();
        if (b.getWidth() <= 0.0f || sampleRate <= 0.0) { binToX.clear(); return; }
        const float nyquistFreq = static_cast<float>(sampleRate) * 0.5f;
        const float minFreq = 20.0f;
        const float logMin = std::log10(minFreq);
        const float logMax = std::log10(nyquistFreq);
        const size_t n = magnitudes.size();
        binToX.resize(n);
        for (size_t i = 0; i < n; ++i) {
            const float frequency = (i * nyquistFreq) / (n > 1 ? (n - 1) : 1);
            const float logFreq = std::log10(juce::jmax(minFreq, frequency));
            binToX[i] = juce::jmap<float>(logFreq, (float)logMin, (float)logMax, b.getX(), b.getRight());
        }
    }

    void spectralSmoothing(std::vector<juce::Point<float>> &points) {
        if(points.size() < 3)
            return;

        const float smoothingCoeff = 0.5f;

        // Lambda for the smoothing operation
        auto applySmoothing = [&](float &state, float input) {
            const float coeff = (input > state) ? smoothingCoeff : 1.0f;
            state = coeff * input + (1.0f - coeff) * state;
        };

        // Forward pass
        float state = points[0].y;
        for(size_t i = 1; i < points.size(); ++i) {
            applySmoothing(state, points[i].y);
            points[i].y = state;
        }

        // Backward pass
        state = points.back().y;
        for(auto i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
            applySmoothing(state, points[i].y);
            points[i].y = state;
        }
    }

    void updateMagnitudes(const auto &newMagnitudes) {
        const float attackCoeff = 0.5f;
        const float releaseCoeff = 0.05f;

        for(size_t i = 0; i < newMagnitudes.size() && i < magnitudes.size(); ++i) {
            float magnitudeDB
             = newMagnitudes[i] > 1e-10f ? 20.0f * std::log10(newMagnitudes[i]) : -100.0f;
            if(magnitudeDB > magnitudes[i]) {
                magnitudes[i] = magnitudes[i] * (1.0f - attackCoeff) + magnitudeDB * attackCoeff;
            } else {
                magnitudes[i] = magnitudes[i] * (1.0f - releaseCoeff) + magnitudeDB * releaseCoeff;
            }
        }
    }

    FFTProcessor<FFTSize> &processor;
    std::vector<float> magnitudes;
    double sampleRate;
    bool isDry;
    float minDB = Parameters::minDBVisualizer;
    float maxDB = Parameters::maxDBVisualizer;
    juce::Colour spectrumColour;
    std::vector<float> binToX;
};
