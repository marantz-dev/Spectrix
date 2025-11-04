#pragma once

#include <JuceHeader.h>

#define FPS 60
#define RT 0.15f
#define DB_FLOOR -48.0f
#define DB_CEILING 6.0f

// #########################
// #                       #
// #  VIRTUAL CLASS METER  #
// #                       #
// #########################
class Meter : public juce::Component, public Timer {
  public:
    Meter() {}
    ~Meter() override {}
    void resized() override {}
    void connectTo(Atomic<float> &targetVariable) { probedSignal = &targetVariable; }
    void flipMeter() { flipped = !flipped; }

  protected:
    void timerCallback() override { repaint(); }
    bool flipped = false;
    Atomic<float> *probedSignal = nullptr;
    float alpha = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Meter)
};

class VolumeMeter : public Meter {
  public:
    VolumeMeter() {
        alpha = exp(-1.0f / (FPS * RT));
        startTimerHz(FPS);
    }

    void paint(juce::Graphics &g) override {
        auto Width = getWidth();
        auto Height = getHeight();

        // Dark background
        g.fillAll(juce::Colour(0xff1a1a1a));

        // Outer border
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRect(0, 0, Width, Height, 1);

        if(probedSignal != nullptr) {
            auto envelopeSnapshot = probedSignal->get();
            probedSignal->set(envelopeSnapshot * alpha);

            auto peak = Decibels::gainToDecibels(envelopeSnapshot);
            // Clamp to our range
            peak = jlimit(DB_FLOOR, DB_CEILING, peak);

            // Calculate how many segments to light up
            int segmentHeight = 2; // Height of each LED segment
            int segmentGap = 1;    // Gap between segments
            int totalSegmentHeight = segmentHeight + segmentGap;

            // Calculate total segments based on available height
            int totalSegments = (Height - 2) / totalSegmentHeight;

            // Calculate active segments based on dB level (-96 to +12 dB range)
            float normalizedLevel = jmap(peak, DB_FLOOR, DB_CEILING, 0.0f, 1.0f);
            int activeSegments = (int)(normalizedLevel * totalSegments);
            activeSegments = jlimit(0, totalSegments, activeSegments);

            // Draw segments
            for(int i = 0; i < totalSegments; i++) {
                // Calculate the dB value this segment represents
                float segmentDB = jmap((float)i / totalSegments, 0.0f, 1.0f, DB_FLOOR, DB_CEILING);

                // Color based on dB thresholds
                juce::Colour segmentColor;
                if(segmentDB < -18.0f) {
                    // Deep green zone (below -18 dB)
                    float t = jmap(segmentDB, DB_FLOOR, -18.0f, 0.0f, 1.0f);
                    segmentColor
                     = juce::Colour(0xff003300).interpolatedWith(juce::Colour(0xff00aa00), t);
                } else if(segmentDB < -12.0f) {
                    // Green zone (-18 to -6 dB)
                    float t = jmap(segmentDB, -18.0f, -6.0f, 0.0f, 1.0f);
                    segmentColor
                     = juce::Colour(0xff00aa00).interpolatedWith(juce::Colour(0xff88ff00), t);
                } else if(segmentDB < 0.0f) {
                    // Yellow/lime zone (-6 to 0 dB)
                    float t = jmap(segmentDB, -6.0f, 0.0f, 0.0f, 1.0f);
                    segmentColor
                     = juce::Colour(0xff88ff00).interpolatedWith(juce::Colour(0xffffff00), t);
                } else {
                    // Red zone (above 0 dB)
                    float t = jmap(segmentDB, 0.0f, DB_CEILING, 0.0f, 1.0f);
                    segmentColor = juce::Colour(0xffff0000)
                                    .interpolatedWith(juce::Colour(0xffff0000).brighter(0.3f), t);
                }

                // Calculate segment position (from bottom or top depending on flip)
                float yPos;
                if(flipped) {
                    yPos = 1.0f + i * totalSegmentHeight;
                } else {
                    yPos = Height - 1.0f - (i + 1) * totalSegmentHeight + segmentGap;
                }

                // Determine if this segment should be lit
                bool isActive = (i < activeSegments);

                if(isActive) {
                    // Active segment - full brightness
                    g.setColour(segmentColor);
                    g.fillRect(2.0f, yPos, Width - 4.0f, (float)segmentHeight);

                    // Add slight glow/highlight on top edge
                    g.setColour(segmentColor.brighter(0.3f));
                    g.fillRect(2.0f, yPos, Width - 4.0f, 1.0f);
                } else {
                    // Inactive segment - very dark version
                    g.setColour(segmentColor.withMultipliedBrightness(0.1f));
                    g.fillRect(2.0f, yPos, Width - 4.0f, (float)segmentHeight);
                }
            }
        }
    }

    void resized() override {}
    void connectTo(Atomic<float> &targetVariable) { probedSignal = &targetVariable; }
    void flipMeter() { flipped = !flipped; }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeMeter)
};
