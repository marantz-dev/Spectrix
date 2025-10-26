#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include "FFTProcessor.h"
#include "juce_graphics/juce_graphics.h"
#include "PluginParameters.h"
#include "UIutils.h"

template <int FFTSize> class SpectrumDisplay : public juce::Component, private juce::Timer {
  public:
    SpectrumDisplay(FFTProcessor<FFTSize> &spectralProcessor, const double sampleRateHz)
        : processor(spectralProcessor), sampleRate(sampleRateHz) {
        magnitudes.resize(processor.getWetMagnitudes().size(), -100.0f);
        startTimerHz(60);
    }

    void paint(juce::Graphics &g) override {
        // g.fillAll(juce::Colours::black);

        const auto &newMagnitudes = processor.getWetMagnitudes();
        if(newMagnitudes.empty())
            return;

        updateMagnitudes(newMagnitudes);

        auto bounds = getLocalBounds();
        juce::Path spectrumPath;
        std::vector<juce::Point<float>> points;
        points.reserve(magnitudes.size());
        points.emplace_back(bounds.getX(), bounds.getBottom());
        float nyquistFreq = static_cast<float>(sampleRate) / 2.0f;
        float minFreq = 20.0f;
        float maxFreq = nyquistFreq;
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);

        for(size_t i = 1; i < magnitudes.size(); ++i) {
            float frequency = (i * nyquistFreq) / (magnitudes.size() - 1);
            float logFreq = std::log10(frequency);
            float x
             = bounds.getX()
               + juce::jmap(logFreq, logMin, logMax, 0.0f, static_cast<float>(bounds.getWidth()));
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

        g.setColour(juce::Colours::cyan.brighter());

        g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));

        spectrumPath.lineTo(points.back().x, bounds.getBottom());
        spectrumPath.lineTo(points[0].x, bounds.getBottom());
        spectrumPath.closeSubPath();

        juce::ColourGradient gradient(juce::Colours::cyan.withAlpha(0.6f), bounds.getX(),
                                      bounds.getY(), juce::Colours::cyan.withAlpha(0.05f),
                                      bounds.getX(), bounds.getBottom(), false);

        g.setGradientFill(gradient);
        g.fillPath(spectrumPath);

        juce::Path strokePath;
        strokePath.startNewSubPath(points[0]);
        for(size_t i = 1; i < points.size(); ++i) {
            strokePath.lineTo(points[i]);
        }

        g.setColour(juce::Colours::cyan);
    }

    void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

  protected:
    void timerCallback() override { repaint(); }

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
    float minDB = Parameters::minDBVisualizer;
    float maxDB = Parameters::maxDBVisualizer;
};
