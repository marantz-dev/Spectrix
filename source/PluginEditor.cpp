#include "PluginEditor.h"
#include "CompressorControls.h"
#include "CompressorModeSection.h"
#include "GainControls.h"
#include "Metering.h"
#include "PluginProcessor.h"
#include "Spectrum.h"
#include "SpectrumSection.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_graphics/juce_graphics.h"

SpectrixAudioProcessorEditor::SpectrixAudioProcessorEditor(SpectrixAudioProcessor &p,
                                                           AudioProcessorValueTreeState &vts)
    : AudioProcessorEditor(&p), audioProcessor(p), compressorControlsSection(vts),
      gainControlSection(vts), compressionModeSection(vts), spectrumSection(p),
      meteringSecttion(p) {
    addAndMakeVisible(spectrumSection);
    addAndMakeVisible(compressorControlsSection);
    addAndMakeVisible(gainControlSection);
    addAndMakeVisible(compressionModeSection);
    addAndMakeVisible(meteringSecttion);

    compressorControlsSection.setLookAndFeel(&theme);
    gainControlSection.setLookAndFeel(&theme);
    compressionModeSection.setLookAndFeel(&theme);

    setSize(1422, 800);
}

void SpectrixAudioProcessorEditor::prepareToPlay(double sr, int sb) {
    spectrumSection.prepareToPlay(sr);
}

SpectrixAudioProcessorEditor::~SpectrixAudioProcessorEditor() {
    compressorControlsSection.setLookAndFeel(nullptr);
    gainControlSection.setLookAndFeel(nullptr);
    compressionModeSection.setLookAndFeel(nullptr);
}

void SpectrixAudioProcessorEditor::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds();
    auto height = bounds.getHeight();

    // g.fillAll(juce::Colours::black.brighter(0.1f));
    g.fillAll(juce::Colours::black.brighter(0.1f));
    g.setColour(juce::Colours::white);

    auto headerBounds = bounds.removeFromTop(height * 0.06f);
    headerBounds.removeFromLeft(40);
    g.setFont(30.0f);
    g.drawFittedText("Spectrix", headerBounds, juce::Justification::left, 1);

    auto topSectionBounds = getLocalBounds();
    topSectionBounds.removeFromBottom(topSectionBounds.getHeight() * 0.3 + 10);

    g.setColour(juce::Colours::whitesmoke);
    juce::Line<float> line(topSectionBounds.getBottomLeft().toFloat(),
                           topSectionBounds.getBottomRight().toFloat());

    g.drawLine(line, 2.0f);
    juce::ColourGradient gradient(Colours::blueviolet.darker(7), 0, 0, // Colour + start point
                                  juce::Colours::black, 0, getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRect(topSectionBounds);
}

void SpectrixAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromTop(bounds.getHeight() * 0.03f);
    bounds.reduce(0, 20);

    auto topSectionBounds = bounds;
    topSectionBounds.removeFromBottom(bounds.getHeight() * 0.3 + 10);
    auto bottomSectionBounds = bounds;
    bottomSectionBounds.removeFromTop(bounds.getHeight() * 0.7 + 10);
    bottomSectionBounds.reduce(20, 0);

    spectrumSection.setBounds(topSectionBounds.withTrimmedRight(bounds.getWidth() * 0.05));
    meteringSecttion.setBounds(topSectionBounds.withTrimmedLeft(bounds.getWidth() * 0.95));

    int trim = bottomSectionBounds.getWidth() * 0.2 + 40;
    compressorControlsSection.setBounds(
     bottomSectionBounds.withTrimmedRight(trim).withTrimmedLeft(trim));

    gainControlSection.setBounds(
     bottomSectionBounds.withTrimmedRight(bottomSectionBounds.getWidth() * 0.8f));
    compressionModeSection.setBounds(
     bottomSectionBounds.withTrimmedLeft(bottomSectionBounds.getWidth() * 0.8f));
}
