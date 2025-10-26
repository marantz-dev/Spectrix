

#pragma once
#include <JuceHeader.h>
#include "PluginParameters.h"
#include "UIutils.h"

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

class GainControlSection : public juce::Component {
  public:
    GainControlSection(AudioProcessorValueTreeState &apvts) : vts(apvts) {
        // Group box
        addAndMakeVisible(compressorSectionBorder);
        compressorSectionBorder.setText("Gain");
        compressorSectionBorder.setTextLabelPosition(juce::Justification::centred);

        // ###################
        // #                 #
        // #  SETUP SLIDERS  #
        // #                 #
        // ###################

        UIutils::setupSlider(inputGainSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minInputGain, Parameters::maxAttack,
                             Parameters::defaultInputGain, Parameters::stepSizeAttack, " db",
                             Parameters::skewFactorInputGain, inputGainLabel, "Input Gain");
        addAndMakeVisible(inputGainSlider);
        addAndMakeVisible(inputGainLabel);

        UIutils::setupSlider(outputGainSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minOutputGain, Parameters::maxRelease,
                             Parameters::defaultOutputGain, Parameters::stepSizeRelease, " db",
                             Parameters::skewFactorOutputGain, outputgainLabel, "output Gain");
        addAndMakeVisible(outputGainSlider);
        addAndMakeVisible(outputgainLabel);

        // ########################
        // #                      #
        // #  SETUP ATTACHEMENTS  #
        // #                      #
        // ########################

        inputGainAttachment.reset(
         new SliderAttachment(vts, Parameters::inputGainID, inputGainSlider));
        outputGainAttachment.reset(
         new SliderAttachment(vts, Parameters::outputGainID, outputGainSlider));
    }

    ~GainControlSection() override {
        inputGainAttachment.reset();
        outputGainAttachment.reset();
    }

    void resized() override {
        auto bounds = getLocalBounds();
        compressorSectionBorder.setBounds(bounds);

        bounds.reduce(30, 30);

        auto knobWidth = bounds.getWidth() / 2;
        inputGainSlider.setBounds(bounds.removeFromLeft(knobWidth).reduced(5));
        outputGainSlider.setBounds(bounds.removeFromLeft(knobWidth).reduced(5));

        UIutils::attachLabel(inputGainLabel, &inputGainSlider);
        UIutils::attachLabel(outputgainLabel, &outputGainSlider);
    }

  private:
    juce::GroupComponent compressorSectionBorder;
    AudioProcessorValueTreeState &vts;

    Slider inputGainSlider;
    Slider outputGainSlider;

    Label inputGainLabel;
    Label outputgainLabel;

    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainControlSection)
};
