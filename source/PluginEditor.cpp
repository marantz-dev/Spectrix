#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Spectrum.h"

SpectrixAudioProcessorEditor::SpectrixAudioProcessorEditor(SpectrixAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      spectrumDisplay(audioProcessor.spectralCompressor, audioProcessor.getSampleRate()),
      // drySpectrumDisplay(audioProcessor.spectralCompressor, audioProcessor.getSampleRate()),
      gainReaductionVisualizer(audioProcessor.spectralCompressor, audioProcessor.getSampleRate()),
      responseCurve(audioProcessor.responseCurve, audioProcessor.getSampleRate()) {
    // Constructor
    setSize(1000, 600);

    // addAndMakeVisible(drySpectrumDisplay);
    // drySpectrumDisplay.setBounds(getLocalBounds());

    addAndMakeVisible(spectrumDisplay);
    spectrumDisplay.setBounds(getLocalBounds());

    addAndMakeVisible(gainReaductionVisualizer);
    gainReaductionVisualizer.setBounds(getLocalBounds());

    addAndMakeVisible(responseCurve);
    responseCurve.setBounds(getLocalBounds());
}

SpectrixAudioProcessorEditor::~SpectrixAudioProcessorEditor() {}

void SpectrixAudioProcessorEditor::updateSpectrumDetail(float newAttack) {
    // spectrumDisplay.updateSpectrumDetail(newAttack);
}

void SpectrixAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);
    g.setFont(juce::FontOptions(15.0f));
}

void SpectrixAudioProcessorEditor::resized() {}
