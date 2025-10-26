
#pragma once
#include <JuceHeader.h>

#define FPS 30
#define RT 0.15f
#define DB_FLOOR -48.0f

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

// ##########################
// #                        #
// #  GAIN REDUCTION METER  #
// #                        #
// ##########################

class GainReductionMeter : public Meter {
  public:
    GainReductionMeter() {
        alpha = exp(-1.0f / (FPS * 0.05));
        startTimerHz(FPS);
    }

    void paint(juce::Graphics &g) override {
        auto Width = getWidth();
        auto Height = getHeight();

        g.fillAll(Colours::black);
        g.setColour(Colours::grey);
        g.drawRect(0, 0, Width, Height, 1);

        if(probedSignal != nullptr) {
            auto envelopeSnapshot = probedSignal->get();

            if(envelopeSnapshot > lastGainReduction) {
                lastGainReduction = envelopeSnapshot;
            } else {
                lastGainReduction = lastGainReduction * alpha + envelopeSnapshot * (1.0f - alpha);
            }
            probedSignal->set(lastGainReduction * alpha);
            float barHeight, offset;
            Colour color;

            auto gainReductionDB = Decibels::gainToDecibels(jmax(0.001f, lastGainReduction));

            barHeight = jmap(gainReductionDB, -48.0f, 0.0f, Height - 2.0f, 0.0f);
            barHeight = jlimit(0.0f, Height - 2.0f, barHeight);

            offset = flipped ? Height - barHeight - 1.0f : 1.0f;
            color = Colours::orange;

            g.setColour(color);
            g.fillRect(1.0f, offset, Width - 2.0f, barHeight);
        }
    }

  private:
    float lastGainReduction = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainReductionMeter)
};

// ##################
// #                #
// #  VOLUME METER  #
// #                #
// ##################

class VolumeMeter : public Meter {
  public:
    VolumeMeter() {
        alpha = exp(-1.0f / (FPS * RT));
        startTimerHz(FPS);
    }

    void paint(juce::Graphics &g) override {
        auto Width = getWidth();
        auto Height = getHeight();

        g.fillAll(Colours::black);
        g.setColour(Colours::grey);
        g.drawRect(0, 0, Width, Height, 1);

        if(probedSignal != nullptr) {
            auto envelopeSnapshot = probedSignal->get();

            probedSignal->set(envelopeSnapshot * alpha);

            float barHeight, offset;
            Colour color;

            auto peak = Decibels::gainToDecibels(envelopeSnapshot);
            barHeight = jmap(peak, DB_FLOOR, 0.0f, 0.0f, Height - 2.0f);
            barHeight = jlimit(0.0f, Height - 2.0f, barHeight);

            color = (peak >= 0.0f) ? Colours::red
                                   : ColourGradient(Colours::darkgreen, 0.0f, 0.0f, Colours::yellow,
                                                    0.0f, Height - 2.0f, false)
                                      .getColourAtPosition(jmap(peak, DB_FLOOR, 0.0f, 0.0f, 1.0f));
            offset = flipped ? 1.0f : Height - barHeight - 1.0f;

            g.setColour(color);
            g.fillRect(1.0f, offset, Width - 2.0f, barHeight);
        }
    }

    void resized() override {}

    void connectTo(Atomic<float> &targetVariable) { probedSignal = &targetVariable; }

    void flipMeter() { flipped = !flipped; }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeMeter)
};
