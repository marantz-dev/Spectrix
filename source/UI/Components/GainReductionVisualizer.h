#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include "SpectralCompressor.h"
#include "UIutils.h"
#include "PluginParameters.h"
#include "juce_graphics/juce_graphics.h"

template <int FFTSize>
class SpectralGainReductionVisualizer : public juce::Component, private juce::Timer {
  public:
    SpectralGainReductionVisualizer(SpectralDynamicsProcessor<FFTSize> &processorRef,
                                    double sampleRateHz)
        : processor(processorRef), sampleRate(sampleRateHz) {
        gainReductions.resize((size_t)FFTSize / 2 + 1, 0.0f);
        startTimerHz(60);
    }

    void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

    void paint(juce::Graphics &g) override {
        // Fetch the latest reductions (copy)
        auto latest = processor.getGainReductionArray();
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

        // Get current mode
        CompressorMode currentMode = getCurrentMode();

        // Use the same dB range as the spectrum display
        const float spectrumMinDB = Parameters::minDBVisualizer;
        const float spectrumMaxDB = Parameters::maxDBVisualizer;

        // Calculate baseline Y position based on mode
        float baselineY = getBaselineYForMode(currentMode, bounds, spectrumMinDB, spectrumMaxDB);
        points.emplace_back(bounds.getX(), baselineY);

        // Build visible points (skip bin 0 which is DC)
        for(size_t i = 1; i < gainReductions.size(); ++i) {
            float freq = (i * nyquist) / (gainReductions.size() - 1);
            if(freq < minFreq)
                continue;

            float logFreq = std::log10(freq);
            float x = bounds.getX() + juce::jmap(logFreq, logMin, logMax, 0.0f, bounds.getWidth());

            // Interpret the gain value based on mode and map to equivalent spectrum dB
            float effectiveDB = interpretGainAsSpectrumDB(gainReductions[i], currentMode,
                                                          spectrumMinDB, spectrumMaxDB);

            // Clamp to spectrum range
            effectiveDB = juce::jlimit(spectrumMinDB, spectrumMaxDB, effectiveDB);

            // Map to Y coordinate using EXACT same mapping as spectrum
            float y = mapSpectrumDBToY(effectiveDB, bounds, spectrumMinDB, spectrumMaxDB);

            points.emplace_back(x, y);
        }

        if(points.size() < 3)
            return;

        // Smooth points (matching SpectrumDisplay approach)
        spectralSmoothing(points);

        // Ensure baseline consistency
        points[0].y = baselineY;

        // Build path with quadratic smoothing
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

        // Get color based on dynamics mode
        juce::Colour fillColour, strokeColour;
        getVisualizerColors(currentMode, fillColour, strokeColour);

        // Stroke the path
        g.setColour(strokeColour);
        g.strokePath(reductionPath, juce::PathStrokeType(2.0f));

        // Fill area between curve and baseline
        if(currentMode != GATE) {
            reductionPath.lineTo(points.back().x, baselineY);
            reductionPath.lineTo(points[0].x, baselineY);
            reductionPath.closeSubPath();
            juce::ColourGradient grad = createGradient(fillColour, bounds, baselineY, currentMode);
            g.setGradientFill(grad);
            g.fillPath(reductionPath);
        }

        // Gradient fill
    }

  private:
    void timerCallback() override { repaint(); }

    CompressorMode getCurrentMode() const { return processor.getCompressorMode(); }

    float getBaselineYForMode(CompressorMode mode, const juce::Rectangle<float> &bounds,
                              float spectrumMinDB, float spectrumMaxDB) const {
        float baselineDB;

        switch(mode) {
        case COMPRESSOR:
        case CLIPPER:
        case GATE:
            // Baseline at 0 dB (no reduction) - middle/upper area
            baselineDB = 0.0f;
            break;

        case EXPANDER:
            // Baseline at bottom (minimum dB) - expander builds up from bottom
            baselineDB = spectrumMinDB;
            break;
        }

        return mapSpectrumDBToY(baselineDB, bounds, spectrumMinDB, spectrumMaxDB);
    }

    float interpretGainAsSpectrumDB(float rawGainValue, CompressorMode mode, float spectrumMinDB,
                                    float spectrumMaxDB) const {
        switch(mode) {
        case COMPRESSOR:
        case CLIPPER:
        case GATE: {
            // rawGainValue is positive gain reduction (e.g., 10 dB reduction)
            // We want to show this as going DOWN from 0 dB baseline
            // So: 0 dB - reduction = negative value
            float resultDB = 0.0f - rawGainValue;
            return resultDB;
        }

        case EXPANDER: {
            // rawGainValue is negative for boost, positive for reduction
            // Baseline is at spectrumMinDB (bottom)
            // Boost (negative values) should go UP from bottom
            // So: minDB + abs(boost) = higher dB value

            // Negate to convert: negative boost → positive offset from bottom
            float offsetFromBottom = -rawGainValue;
            float resultDB = spectrumMinDB + offsetFromBottom;
            return resultDB;
        }

        default: return 0.0f;
        }
    }

    float mapSpectrumDBToY(float dB, const juce::Rectangle<float> &bounds, float minDB,
                           float maxDB) const {
        // EXACT same mapping as SpectrumDisplay::paint()
        // 1. Apply DBWarp
        float warpedDB = DBWarp(dB);

        // 2. Map warped value to Y coordinate
        // Note: Y is inverted (top = maxDB, bottom = minDB)
        float y = juce::jmap(warpedDB, minDB, maxDB, static_cast<float>(bounds.getBottom()),
                             static_cast<float>(bounds.getY()));

        return y;
    }

    void getVisualizerColors(CompressorMode mode, juce::Colour &fillColour,
                             juce::Colour &strokeColour) const {
        switch(mode) {
        case COMPRESSOR:
            fillColour = juce::Colours::red.withAlpha(0.5f);
            strokeColour = juce::Colours::red.withAlpha(0.3f);
            break;

        case CLIPPER:
            fillColour = juce::Colours::orange.withAlpha(0.5f);
            strokeColour = juce::Colours::orange.withAlpha(0.3f);
            break;

        case EXPANDER:
            fillColour = juce::Colours::green.withAlpha(0.5f);
            strokeColour = juce::Colours::green.withAlpha(0.3f);
            break;

        case GATE:
            fillColour = juce::Colours::transparentWhite;
            ;
            strokeColour = juce::Colours::blue.withAlpha(0.1f);
            break;
        }
    }

    juce::ColourGradient
    createGradient(const juce::Colour &baseColour, const juce::Rectangle<float> &bounds,
                   float baselineY, CompressorMode mode) const {
        float gradientEndY;

        if(mode == EXPANDER) {
            // Expander: baseline at bottom, gradient goes upward
            gradientEndY = bounds.getY();
        } else {
            // Reduction modes: baseline at top/middle, gradient goes downward
            gradientEndY = bounds.getBottom();
        }

        return juce::ColourGradient(baseColour.withAlpha(0.6f), bounds.getX(), baselineY,
                                    baseColour.withAlpha(0.0f), bounds.getX(), gradientEndY, false);
    }

    // Attack/release smoothing of incoming dB values
    void updateGainReduction(const std::array<float, (size_t)FFTSize / 2 + 1> &newData) {
        const float attackCoeff = 0.8f;   // Fast attack
        const float releaseCoeff = 0.92f; // Slower release

        for(size_t i = 0; i < newData.size() && i < gainReductions.size(); ++i) {
            float incoming = newData[i];

            // Use attack when magnitude is increasing (more reduction/boost)
            bool isIncreasing = std::abs(incoming) > std::abs(gainReductions[i]);

            if(isIncreasing)
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
        for(int i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
            applySmoothing(state, points[i].y);
            points[i].y = state;
        }
    }

    SpectralDynamicsProcessor<FFTSize> &processor;
    std::vector<float> gainReductions;
    double sampleRate;
};
