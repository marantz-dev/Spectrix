#pragma once
#include <JuceHeader.h>
#include "PluginParameters.h"
#include "UIutils.h"

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

        struct FreqLabel {
            float freq;
            juce::String label;
            bool isMajor;
        };

        const FreqLabel freqLabels[]
         = {{30.f, "", false},     {40.f, "", false},      {50.f, "50", true},
            {60.f, "", false},     {70.f, "", false},      {80.f, "", false},
            {90.f, "", false},     {100.f, "100", true},   {200.f, "200", true},
            {300.f, "", false},    {400.f, "", false},     {500.f, "500", true},
            {600.f, "", false},    {700.f, "", false},     {800.f, "", false},
            {900.f, "", false},    {1000.f, "1k", true},   {2000.f, "2k", true},
            {3000.f, "", false},   {4000.f, "", false},    {5000.f, "5k", true},
            {6000.f, "", false},   {7000.f, "", false},    {8000.f, "", false},
            {9000.f, "", false},   {10000.f, "10k", true}, {15000.f, "", false},
            {20000.f, "20k", true}};

        g.setFont(10.f);

        // Vertical frequency grid (existing behavior)
        for(const auto &freqLabel : freqLabels) {
            if(freqLabel.freq > maxFreq || freqLabel.freq < minFreq)
                continue;

            float x = logFrequencyToX(freqLabel.freq);

            if(freqLabel.isMajor) {
                g.setColour(juce::Colour(70, 75, 85));
                g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());

                if(!freqLabel.label.isEmpty()) {
                    g.setColour(juce::Colour(150, 155, 165));
                    g.drawText(freqLabel.label, juce::roundToInt(x) - 20,
                               juce::roundToInt(bounds.getBottom()) - 18, 40, 15,
                               juce::Justification::centred);
                }
            } else {
                g.setColour(juce::Colour(50, 55, 65).withAlpha(0.4f));
                g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
            }
        }

        // Horizontal dB grid (new)
        {
            const float minDB = Parameters::minDBVisualizer;
            const float maxDB = Parameters::maxDBVisualizer;

            auto dbToY = [&](float db) {
                // Match the spectrum mapping: warp then map warpedDB -> screen Y
                float warped = DBWarp(db);
                return juce::jmap<float>(warped, minDB, maxDB, bounds.getBottom(), bounds.getY());
            };

            // Draw lines every 10 dB; mark majors every 20 dB
            const int step = 10;
            int start = static_cast<int>(std::ceil(minDB / (float)step) * step);
            int end = static_cast<int>(std::floor(maxDB / (float)step) * step);

            for(int db = start; db <= end; db += step) {
                float y = dbToY((float)db);
                bool isMajor = (db % 20 == 0);

                if(isMajor)
                    g.setColour(juce::Colour(70, 75, 85));
                else
                    g.setColour(juce::Colour(50, 55, 65).withAlpha(0.4f));

                g.drawHorizontalLine(juce::roundToInt(y), bounds.getX(), bounds.getRight());

                if(isMajor) {
                    g.setColour(juce::Colour(150, 155, 165));
                    g.drawText(juce::String(db) + " dB", juce::roundToInt(bounds.getX()) + 4,
                               juce::roundToInt(y) - 15, 48, 14, juce::Justification::left);
                }
            }
        }

        // Top fade (existing)
        {
            float fadeHeight = 40.0f; // adjust to taste

            juce::ColourGradient fadeGrad(
             juce::Colour(21, 9, 37).withAlpha(1.0f), // fully opaque at very top
             bounds.getX(), bounds.getY(),
             juce::Colour(21, 9, 37).withAlpha(0.0f), // fully transparent at fadeHeight
             bounds.getX(), bounds.getY() + fadeHeight, false);

            g.setGradientFill(fadeGrad);
            g.fillRect(bounds.withHeight(fadeHeight));
        }
    }

  private:
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
    double minFreq = 20.0;
    double maxFreq = 22050.0;
    double logMin = std::log10(20.0);
    double logMax = std::log10(22050.0);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumGrid)
};
