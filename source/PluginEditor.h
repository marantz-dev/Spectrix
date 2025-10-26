#pragma once

#include "CompressorControls.h"
#include "CompressorModeSection.h"
#include "GainControls.h"
#include "Metering.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include "ResponseCurve.h"
#include "SpectrumSection.h"
#include "Theme.h"
#include "juce_audio_processors/juce_audio_processors.h"

class SpectrixAudioProcessorEditor : public juce::AudioProcessorEditor {
  public:
    SpectrixAudioProcessorEditor(SpectrixAudioProcessor &, AudioProcessorValueTreeState &vts);
    ~SpectrixAudioProcessorEditor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    SpectrumSection spectrumSection;

    CompressorSection compressorControlsSection;
    GainControlSection gainControlSection;
    CompressionModeSection compressionModeSection;
    MeteringSection meteringSecttion;

    SpectrixAudioProcessor &audioProcessor;

    juce::Image pluginTitleImage;

    Theme theme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrixAudioProcessorEditor)
};
