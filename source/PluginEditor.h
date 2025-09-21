#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class SpectrixAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  SpectrixAudioProcessorEditor(SpectrixAudioProcessor &);
  ~SpectrixAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  SpectrixAudioProcessor &audioProcessor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrixAudioProcessorEditor)
};
