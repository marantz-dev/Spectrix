#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class FFTProcessorAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  FFTProcessorAudioProcessorEditor(FFTProcessorAudioProcessor &);
  ~FFTProcessorAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  FFTProcessorAudioProcessor &audioProcessor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessorAudioProcessorEditor)
};
