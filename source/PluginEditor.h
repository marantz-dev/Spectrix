#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>
#include "Spectrum.h"
#include "PluginParameters.h"

class SpectrixAudioProcessorEditor : public juce::AudioProcessorEditor {
    public:
      SpectrixAudioProcessorEditor(SpectrixAudioProcessor &);
      ~SpectrixAudioProcessorEditor() override;

      //==============================================================================
      void paint(juce::Graphics &) override;
      void resized() override;

    private:
      SpectrixAudioProcessor &audioProcessor;
      SpectrumDisplay<Parameters::FFT_SIZE> spectrumDisplay;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrixAudioProcessorEditor)
};
