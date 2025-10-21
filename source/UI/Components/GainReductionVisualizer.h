#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include "SpectralCompressor.h"

template <int FFTSize>
class SpectralGainReductionVisualizer : public juce::Component, private juce::Timer {
  public:
    SpectralGainReductionVisualizer(SpectralCompressor<FFTSize> &compressorRef, double sampleRateHz)
        : compressor(compressorRef), sampleRate(sampleRateHz) {
        gainReductions.resize((size_t)FFTSize / 2 + 1, 0.0f);
        startTimerHz(60);
    }

    void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

    void paint(juce::Graphics &g) override {
        // fetch the latest reductions (copy)
        auto latest = compressor.getGainReductionArray();
        if(latest.size() == 0)
            return;

        updateGainReduction(latest);

        auto bounds = getLocalBounds().toFloat();
        juce::Path reductionPath;
        std::vector<juce::Point<float>> points;
        points.reserve(gainReductions.size());

        const float nyquist = static_cast<float>(sampleRate) * 0.5f;
        const float minFreq = 20.0f;
        const float logMin = std::log10(minFreq);
        const float logMax = std::log10(nyquist);

        // baseline left-bottom point (start the filled area)
        points.emplace_back(bounds.getX(), bounds.getBottom());

        // visual range: 0 dB (no reduction) -> maxReduction dB (full visible)
        const float maxReduction = 96.0f; // clamp display range to 0..40 dB

        // Build visible points (skip bin 0 which is DC)
        for(size_t i = 1; i < gainReductions.size(); ++i) {
            // freq mapping (same approach as your spectrum display)
            float freq = (i * nyquist) / (gainReductions.size() - 1);
            if(freq < minFreq)
                continue;

            float logFreq = std::log10(freq);
            float x = bounds.getX() + juce::jmap(logFreq, logMin, logMax, 0.0f, bounds.getWidth());

            // gainReductions[] is positive dB of reduction (e.g. 5.0 meaning -5 dB applied)
            float dB = gainReductions[i];
            dB = juce::jlimit(0.0f, maxReduction, dB);

            float y = juce::jmap(dB, 0.0f, maxReduction, bounds.getCentreY(), bounds.getBottom());

            points.emplace_back(x, y);
        }

        if(points.size() < 3)
            return;

        // Smooth points (small smoothing pass similar to SpectrumDisplay)
        spectralSmoothing(points);

        // Build path with quadratic smoothing
        points[0].y = bounds.getCentreY(); // ensure baseline start
        reductionPath.startNewSubPath(points[0]);
        for(size_t i = 1; i < points.size(); ++i) {
            if(i == 1 || i == points.size() - 1) {
                reductionPath.lineTo(points[i]);
            } else {
                auto midPoint = juce::Point<float>((points[i - 1].x + points[i].x) * 0.5f,
                                                   (points[i - 1].y + points[i].y) * 0.5f);
                reductionPath.quadraticTo(points[i - 1], midPoint);
            }
        }

        // Stroke
        g.setColour(juce::Colours::red.withAlpha(0.3f));
        g.strokePath(reductionPath, juce::PathStrokeType(2.0f));

        // Fill area under curve
        reductionPath.lineTo(points.back().x, bounds.getBottom());
        reductionPath.lineTo(points[0].x, bounds.getBottom());
        reductionPath.closeSubPath();

        juce::ColourGradient grad(juce::Colours::red.withAlpha(0.55f), bounds.getX(), bounds.getY(),
                                  juce::Colours::black.withAlpha(0.0f), bounds.getX(),
                                  bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(reductionPath);
    }

  private:
    void timerCallback() override { repaint(); }

    // simple attack/release smoothing of incoming dB values
    void updateGainReduction(const std::array<float, (size_t)FFTSize / 2 + 1> &newData) {
        const float attackCoeff = 0.2f;
        const float releaseCoeff = 0.05f;

        for(size_t i = 0; i < newData.size() && i < gainReductions.size(); ++i) {
            float incoming = newData[i];
            if(incoming > gainReductions[i])
                gainReductions[i]
                 = gainReductions[i] * (1.0f - attackCoeff) + incoming * attackCoeff;
            else
                gainReductions[i]
                 = gainReductions[i] * (1.0f - releaseCoeff) + incoming * releaseCoeff;
        }
    }

    void spectralSmoothing(std::vector<juce::Point<float>> &points) {
        if(points.size() < 3)
            return;

        const float smoothingCoeff = 0.5f;
        // forward/backward pass similar to SpectrumDisplay
        auto applySmoothing = [&](float &state, float input) {
            const float coeff = (input > state) ? smoothingCoeff : 1.0f;
            state = coeff * input + (1.0f - coeff) * state;
        };

        float state = points[0].y;
        for(size_t i = 1; i < points.size(); ++i) {
            applySmoothing(state, points[i].y);
            points[i].y = state;
        }

        state = points.back().y;
        for(int i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
            applySmoothing(state, points[i].y);
            points[i].y = state;
        }
    }

    SpectralCompressor<FFTSize> &compressor;
    std::vector<float> gainReductions;
    double sampleRate;
};
