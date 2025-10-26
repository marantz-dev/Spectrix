#pragma once

#include "CompressorControls.h"
#include "CompressorModeSection.h"
#include "GainControls.h"
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
    void paint(juce::Graphics &) override;
    void resized() override;

    void updateSpectrumDetail(float newAttack);

  private:
    SpectrumSection spectrumSection;

    CompressorSection compressorControlsSection;
    GainControlSection gainControlSection;
    CompressionModeSection compressionModeSection;

    SpectrixAudioProcessor &audioProcessor;

    Theme theme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrixAudioProcessorEditor)
};
