#pragma once
#include <JuceHeader.h>
#include "CompressorControls.h"
#include "PluginParameters.h"
#include "UIutils.h"
#include "Theme.h" // <--- Make sure to include your Theme file

class CompressionModeSection : public juce::Component {
  public:
    CompressionModeSection(AudioProcessorValueTreeState &apvts, CompressorSection &compSection)
        : vts(apvts), dependentSection(compSection) {
        setLookAndFeel(&theme);

        addAndMakeVisible(compressorModeBorder);
        compressorModeBorder.setText("MODE");
        compressorModeBorder.setTextLabelPosition(juce::Justification::centred);
        compressorModeBorder.setColour(juce::GroupComponent::textColourId,
                                       juce::Colours::white.withAlpha(0.8f));
        compressorModeBorder.setColour(juce::GroupComponent::outlineColourId,
                                       juce::Colours::white.withAlpha(0.2f));

        UIutils::setupToggleButton(compressorButton, "Compressor");
        addAndMakeVisible(compressorButton);

        UIutils::setupToggleButton(expanderButton, "Expander");
        addAndMakeVisible(expanderButton);

        UIutils::setupToggleButton(clipperButton, "Clipper");
        addAndMakeVisible(clipperButton);

        UIutils::setupToggleButton(gateButton, "Gate");
        addAndMakeVisible(gateButton);

        compressorButton.onClick = [this]() { setMode(0); };
        expanderButton.onClick = [this]() { setMode(1); };
        clipperButton.onClick = [this]() { setMode(2); };
        gateButton.onClick = [this]() { setMode(3); };

        updateButtons();
    }

    ~CompressionModeSection() override {
        setLookAndFeel(nullptr); // Clean up L&F
    }

    void resized() override {
        auto bounds = getLocalBounds();
        compressorModeBorder.setBounds(bounds);

        // Calculate layout with margins
        int horizontalMargin = 20;
        int verticalMargin = 25;
        auto buttonArea = bounds.reduced(horizontalMargin, verticalMargin);

        // Distribute buttons evenly
        int numButtons = 4;
        int buttonHeight = buttonArea.getHeight() / numButtons;

        compressorButton.setBounds(buttonArea.removeFromTop(buttonHeight));
        expanderButton.setBounds(buttonArea.removeFromTop(buttonHeight));
        clipperButton.setBounds(buttonArea.removeFromTop(buttonHeight));
        gateButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    }

  private:
    void setMode(int index) {
        if(auto *param = dynamic_cast<juce::AudioParameterChoice *>(

            vts.getParameter(Parameters::compressorModeID))) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(index));
            dependentSection.updateEnabled();
            param->endChangeGesture();
        }
        updateButtons();
    }

    void updateButtons() {
        auto *param = dynamic_cast<juce::AudioParameterChoice *>(
         vts.getParameter(Parameters::compressorModeID));
        if(!param)
            return;

        int value = param->getIndex();
        compressorButton.setToggleState(value == 0, juce::dontSendNotification);
        expanderButton.setToggleState(value == 1, juce::dontSendNotification);
        clipperButton.setToggleState(value == 2, juce::dontSendNotification);
        gateButton.setToggleState(value == 3, juce::dontSendNotification);
    }

    // Member Variables
    Theme theme; // <--- The custom look and feel
    juce::GroupComponent compressorModeBorder;
    AudioProcessorValueTreeState &vts;

    juce::ToggleButton compressorButton;
    juce::ToggleButton expanderButton;
    juce::ToggleButton gateButton;
    juce::ToggleButton clipperButton;
    CompressorSection &dependentSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressionModeSection)
};
