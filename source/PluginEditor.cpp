#include "PluginEditor.h"
#include "PluginProcessor.h"

FFTProcessorAudioProcessorEditor::FFTProcessorAudioProcessorEditor(FFTProcessorAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  setSize(400, 300);
}

FFTProcessorAudioProcessorEditor::~FFTProcessorAudioProcessorEditor() {}

void FFTProcessorAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
  g.setFont(juce::FontOptions(15.0f));
  g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void FFTProcessorAudioProcessorEditor::resized() {}
