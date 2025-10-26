

#pragma once
#include <JuceHeader.h>
#include "PluginParameters.h"
#include "UIutils.h"
#include "juce_gui_basics/juce_gui_basics.h"

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

class CompressionModeSection : public juce::Component {
  public:
    CompressionModeSection(AudioProcessorValueTreeState &apvts) : vts(apvts) {
        // Group box
        addAndMakeVisible(compressorSectionBorder);
        compressorSectionBorder.setText("MODE");
        compressorSectionBorder.setTextLabelPosition(juce::Justification::centred);

        UIutils::setupToggleButton(compressorButton, "Compressor");
        addAndMakeVisible(compressorButton);

        UIutils::setupToggleButton(clipperButton, "Clipper");
        clipperButton.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(clipperButton);

        UIutils::setupToggleButton(gateButton, "Gate");
        addAndMakeVisible(gateButton);

        compressorButton.onClick = [this]() {
            if(auto *param = dynamic_cast<juce::AudioParameterChoice *>(
                vts.getParameter(Parameters::compressorModeID))) {
                param->setValueNotifyingHost(param->convertTo0to1(0)); // "Compressor"
            }
            updateButtons();
        };

        clipperButton.onClick = [this]() {
            if(auto *param = dynamic_cast<juce::AudioParameterChoice *>(
                vts.getParameter(Parameters::compressorModeID))) {
                param->setValueNotifyingHost(param->convertTo0to1(1)); // "Clipper"
            }
            updateButtons();
        };

        gateButton.onClick = [this]() {
            if(auto *param = dynamic_cast<juce::AudioParameterChoice *>(
                vts.getParameter(Parameters::compressorModeID))) {
                param->setValueNotifyingHost(param->convertTo0to1(2)); // "Gate"
            }
            updateButtons();
        };
    }
    ~CompressionModeSection() override {}
    void resized() override {
        auto bounds = getLocalBounds();
        compressorSectionBorder.setBounds(bounds);
        bounds.reduce(30, 30);
        int buttonheight = bounds.getHeight() / 3 - 10;

        compressorButton.setBounds(bounds.withHeight(buttonheight));

        bounds.reduce(0, buttonheight + 20);
        clipperButton.setBounds(bounds.withHeight(buttonheight));

        bounds.reduce(0, buttonheight + 20);
        gateButton.setBounds(bounds.withHeight(buttonheight));
    }

  private:
    void updateButtons() {
        auto *param
         = dynamic_cast<AudioParameterChoice *>(vts.getParameter(Parameters::compressorModeID));
        int value = param->getIndex();
        compressorButton.setToggleState(value == 0, juce::dontSendNotification);
        clipperButton.setToggleState(value == 1, juce::dontSendNotification);
        gateButton.setToggleState(value == 2, juce::dontSendNotification);
    }

    juce::GroupComponent compressorSectionBorder;
    AudioProcessorValueTreeState &vts;

    ToggleButton compressorButton;
    ToggleButton gateButton;
    ToggleButton clipperButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressionModeSection)
};
