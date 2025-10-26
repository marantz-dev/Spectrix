

#pragma once
#include "PluginProcessor.h"
#include "Meters.h"
#include <JuceHeader.h>

class MeteringSection : public juce::Component {
  public:
    MeteringSection(SpectrixAudioProcessor &p) : audioProcessor(p) {
        addAndMakeVisible(inputMeter);
        inputMeter.connectTo(p.inputProbe);

        addAndMakeVisible(outputMeter);
        outputMeter.connectTo(p.outputProbe);

        inputLabel.setText("IN", juce::dontSendNotification);
        inputLabel.setJustificationType(juce::Justification::centred);

        outputLabel.setText("OUT", juce::dontSendNotification);
        outputLabel.setJustificationType(juce::Justification::centred);
    }

    void resized() override {
        auto bounds = getLocalBounds();

        auto meterWidth = bounds.getWidth() / 2;

        inputMeter.setBounds(getLocalBounds().withTrimmedRight(bounds.getWidth() * 0.5f));
        outputMeter.setBounds(getLocalBounds().withTrimmedLeft(bounds.getWidth() * 0.5f));
    }

  private:
    SpectrixAudioProcessor &audioProcessor;

    VolumeMeter inputMeter;
    VolumeMeter outputMeter;

    Label inputLabel;
    Label outputLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeteringSection)
};
