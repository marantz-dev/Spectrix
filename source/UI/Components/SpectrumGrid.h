#pragma once
#include <JuceHeader.h>

class SpectrumGrid : public juce::Component {
  public:
    SpectrumGrid(double sampleRateHz) : sampleRate(sampleRateHz) { updateLogFreqBounds(); }

    void setSampleRate(double newSampleRate) {
        sampleRate = newSampleRate;
        updateLogFreqBounds();
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();

        float freq500 = 500.0f;
        drawLine(g, freq500);
    }

  private:
    void drawLine(juce::Graphics &g, float frequency) {
        auto bounds = getLocalBounds().toFloat();
        float x = logFrequencyToX(frequency);
        g.setColour(juce::Colours::red);
        g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
    }

    float logFrequencyToX(float frequency) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        float logFreq = std::log10(frequency);
        return juce::jmap<float>(logFreq, (float)logMin, (float)logMax, bounds.getX(),
                                 bounds.getRight());
    }

    void updateLogFreqBounds() {
        minFreq = 20.0;
        maxFreq = sampleRate * 0.5;
        logMin = std::log10(minFreq);
        logMax = std::log10(maxFreq);
    }

    double sampleRate;
    double minFreq;
    double maxFreq;
    double logMin;
    double logMax;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumGrid)
};
