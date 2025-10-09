#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Spectrum.h"

SpectrixAudioProcessorEditor::SpectrixAudioProcessorEditor(SpectrixAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      spectrumDisplay(audioProcessor.spectralCompressor, audioProcessor.getSampleRate()) {
    // Constructor
    setSize(1000, 600);
    addAndMakeVisible(spectrumDisplay);
    spectrumDisplay.setBounds(getLocalBounds());

    // #######################################
    // #                                     #
    // #  UNCOMMENT THIS FOR RESPONSE CURVE  #
    // #                                     #
    // #######################################

    addAndMakeVisible(responseCurve);
    responseCurve.setBounds(getLocalBounds());
}

SpectrixAudioProcessorEditor::~SpectrixAudioProcessorEditor() {}

void SpectrixAudioProcessorEditor::updateSpectrumDetail(float newAttack) {
    spectrumDisplay.updateSpectrumDetail(newAttack);
}

void SpectrixAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setFont(juce::FontOptions(15.0f));
    g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SpectrixAudioProcessorEditor::resized() {}
