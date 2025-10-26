#pragma once
#include "PluginParameters.h"
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class SpectrumGrid : public juce::Component {
  public:
    SpectrumGrid(double sampleRateHz) : sampleRate(sampleRateHz) {}

    void setSampleRate(double newSampleRate) {
        sampleRate = newSampleRate;
        repaint();
    }

    void setGridSettings(float minFreqHz = 20.0f, float maxFreqHz = 20000.0f) {
        minFreq = minFreqHz;
        maxFreq = maxFreqHz;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        const int numBins = Parameters::FFT_SIZE / 2 + 1;
        float nyquist = static_cast<float>(sampleRate) / 2.0f;

        // Draw vertical frequency lines
        g.setFont(10.0f);

        for(int k = 1; k < numBins; ++k) {
            float freq = binFrequency(k);
            if(freq < minFreq || freq > std::min(maxFreq, nyquist))
                continue;

            float x = frequencyToX(freq, bounds);

            // Decide if major or minor line
            bool isMajor = isMajorFrequency(freq);

            if(isMajor) {
                g.setColour(juce::Colour(70, 75, 85));
                g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());

                juce::String label = formatLabel(freq);
                g.setColour(juce::Colour(150, 155, 165));
                g.drawText(label, juce::roundToInt(x) - 20,
                           juce::roundToInt(bounds.getBottom()) - 18, 40, 15,
                           juce::Justification::centred);
            } else {
                g.setColour(juce::Colour(50, 55, 65).withAlpha(0.4f));
                g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
            }
        }
    }

  private:
    double sampleRate;
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;

    // Compute frequency of a given FFT bin
    float binFrequency(int binIndex) const {
        return static_cast<float>(binIndex) * static_cast<float>(sampleRate)
               / static_cast<float>(Parameters::FFT_SIZE);
    }

    // Map frequency to X (logarithmic)
    float frequencyToX(float freq, const juce::Rectangle<float> &bounds) const {
        float logMin = std::log10(minFreq);
        float logMax = std::log10(std::min(maxFreq, static_cast<float>(sampleRate) / 2.0f));
        float logFreq = std::log10(freq);
        return bounds.getX() + juce::jmap(logFreq, logMin, logMax, 0.0f, bounds.getWidth());
    }

    // Decide if this frequency should be a major grid line
    bool isMajorFrequency(float freq) const {
        const std::vector<float> majorFreqs
         = {20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f};
        for(auto f : majorFreqs) {
            if(std::abs(freq - f) < (binFrequency(1) / 2.0f)) // closest bin
                return true;
        }
        return false;
    }

    // Format frequency labels
    juce::String formatLabel(float freq) const {
        if(freq >= 1000.0f)
            return juce::String(freq / 1000.0f, 0) + "k";
        else
            return juce::String(static_cast<int>(freq));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumGrid)
};
