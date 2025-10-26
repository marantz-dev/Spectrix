

#pragma once
#include <JuceHeader.h>
#include "PluginParameters.h"
#include "UIutils.h"

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

class CompressorSection : public juce::Component {
  public:
    CompressorSection(AudioProcessorValueTreeState &apvts) : vts(apvts) {
        // Group box
        addAndMakeVisible(compressorSectionBorder);
        compressorSectionBorder.setText("Compressor");
        compressorSectionBorder.setTextLabelPosition(juce::Justification::centred);

        // ###################
        // #                 #
        // #  SETUP SLIDERS  #
        // #                 #
        // ###################

        UIutils::setupSlider(inputGainSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minAttack, Parameters::maxAttack,
                             Parameters::defaultAttackTime, Parameters::stepSizeAttack, " ms",
                             Parameters::skewFactorAttack, inputGainLabel, "Attack");
        addAndMakeVisible(inputGainSlider);
        addAndMakeVisible(inputGainLabel);

        UIutils::setupSlider(outputGainSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minRelease, Parameters::maxRelease,
                             Parameters::defaultReleaseTime, Parameters::stepSizeRelease, " ms",
                             Parameters::skewFactorRelease, outputgainLabel, "Release");
        addAndMakeVisible(outputGainSlider);
        addAndMakeVisible(outputgainLabel);

        UIutils::setupSlider(curveShiftSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minCurveShift, Parameters::maxCurveShift,
                             Parameters::defaultCurveShiftDB, Parameters::stepSizeCurveShift, " dB",
                             Parameters::skewFactorCurveShift, thresholdLabel, "Curve Shift");
        addAndMakeVisible(curveShiftSlider);
        addAndMakeVisible(thresholdLabel);

        UIutils::setupSlider(ratioSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minRatio, Parameters::maxRatio, Parameters::defaultRatio,
                             Parameters::stepSizeRatio, ":1", Parameters::skewFactorRatio,
                             ratioLabel, "Ratio");
        addAndMakeVisible(ratioSlider);
        addAndMakeVisible(ratioLabel);

        UIutils::setupSlider(kneeSlider, juce::Slider::RotaryHorizontalVerticalDrag,
                             Parameters::minKnee, Parameters::maxKnee, Parameters::defaultKneeWidth,
                             Parameters::stepSizeKnee, " dB", Parameters::skewFactorKnee, kneeLabel,
                             "Knee Width");
        kneeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        kneeSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
        addAndMakeVisible(kneeSlider);
        addAndMakeVisible(kneeLabel);

        // ###################
        // #                 #
        // #  SETUP BUTTONS  #
        // #                 #
        // ###################

        // ########################
        // #                      #
        // #  SETUP ATTACHEMENTS  #
        // #                      #
        // ########################

        inputGainAttachment.reset(
         new SliderAttachment(vts, Parameters::attackTimeID, inputGainSlider));
        outputGainAttachment.reset(
         new SliderAttachment(vts, Parameters::releaseTimeID, outputGainSlider));
        thresholdAttachment.reset(
         new SliderAttachment(vts, Parameters::curveShiftDBID, curveShiftSlider));
        ratioAttachment.reset(new SliderAttachment(vts, Parameters::ratioID, ratioSlider));
        kneeAttachment.reset(new SliderAttachment(vts, Parameters::kneeWidthID, kneeSlider));
    }

    ~CompressorSection() override {
        inputGainAttachment.reset();
        outputGainAttachment.reset();
        thresholdAttachment.reset();
        ratioAttachment.reset();
        kneeAttachment.reset();
    }

    void resized() override {
        auto bounds = getLocalBounds();
        compressorSectionBorder.setBounds(bounds);

        bounds.reduce(30, 30);

        auto knobWidth = bounds.getWidth() / 5;
        inputGainSlider.setBounds(bounds.removeFromLeft(knobWidth).reduced(5));
        outputGainSlider.setBounds(bounds.removeFromLeft(knobWidth).reduced(5));
        curveShiftSlider.setBounds(bounds.removeFromLeft(knobWidth).reduced(5));
        ratioSlider.setBounds(bounds.removeFromLeft(knobWidth).reduced(5));
        kneeSlider.setBounds(bounds.reduced(5));

        UIutils::attachLabel(inputGainLabel, &inputGainSlider);
        UIutils::attachLabel(outputgainLabel, &outputGainSlider);
        UIutils::attachLabel(thresholdLabel, &curveShiftSlider);
        UIutils::attachLabel(ratioLabel, &ratioSlider);
        UIutils::attachLabel(kneeLabel, &kneeSlider);
    }

  private:
    juce::GroupComponent compressorSectionBorder;
    AudioProcessorValueTreeState &vts;

    Slider inputGainSlider;
    Slider outputGainSlider;
    Slider curveShiftSlider;
    Slider ratioSlider;
    Slider kneeSlider;

    Label inputGainLabel;
    Label outputgainLabel;
    Label thresholdLabel;
    Label ratioLabel;
    Label kneeLabel;

    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment;
    std::unique_ptr<SliderAttachment> kneeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorSection)
};
