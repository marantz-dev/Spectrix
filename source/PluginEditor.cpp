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
#include <iostream>

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

    pluginTitleImage
     = juce::ImageFileFormat::loadFrom(juce::File("/Users/riccardomarantonio/Desktop/LOGO.png"));

    setSize(1422, 800);
    // setResizable(true, true);
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

    g.fillAll(juce::Colours::black.brighter(0.1f));

    auto topSectionBounds = getLocalBounds();
    topSectionBounds.removeFromBottom(topSectionBounds.getHeight() * 0.3 + 10);

    g.setColour(juce::Colours::whitesmoke);
    juce::Line<float> line(topSectionBounds.getBottomLeft().toFloat(),
                           topSectionBounds.getBottomRight().toFloat());
    g.drawLine(line, 2.0f);

    juce::ColourGradient gradient(juce::Colours::blueviolet.darker(5), 0, 0,
                                  juce::Colours::cyan.darker(8), 0, getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    if(pluginTitleImage.isValid()) {
        g.drawImage(pluginTitleImage, 10, 10, 266, 55, 0, 0, pluginTitleImage.getWidth(),
                    pluginTitleImage.getHeight());
    } else {
        g.setColour(juce::Colours::white);
        g.setFont(30.0f);
        g.drawText("Spectrix", 30, 30, 300, 50, juce::Justification::left);
    }
}

void SpectrixAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(20);

    auto topSectionBounds = bounds;
    topSectionBounds.removeFromBottom(bounds.getHeight() * 0.3 + 10);
    auto bottomSectionBounds = bounds;
    bottomSectionBounds.removeFromTop(bounds.getHeight() * 0.7 + 10);
    bottomSectionBounds.reduce(20, 0);

    meteringSecttion.setBounds(topSectionBounds.withTrimmedLeft(bounds.getWidth() * 0.975));
    spectrumSection.setBounds(topSectionBounds.withTrimmedRight(bounds.getWidth() * 0.025)
                               .removeFromBottom(topSectionBounds.getHeight() - 75));

    int trim = bottomSectionBounds.getWidth() * 0.2 + 40;
    compressorControlsSection.setBounds(
     bottomSectionBounds.withTrimmedRight(trim).withTrimmedLeft(trim));

    gainControlSection.setBounds(
     bottomSectionBounds.withTrimmedRight(bottomSectionBounds.getWidth() * 0.8f));
    compressionModeSection.setBounds(
     bottomSectionBounds.withTrimmedLeft(bottomSectionBounds.getWidth() * 0.8f));
}
