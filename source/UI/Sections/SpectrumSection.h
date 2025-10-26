

#pragma once
#include <JuceHeader.h>
#include "GainReductionVisualizer.h"
#include "PluginParameters.h"
#include "PluginProcessor.h"
#include "ResponseCurve.h"
#include "Spectrum.h"
#include "SpectrumGrid.h"

class SpectrumSection : public juce::Component {
  public:
    SpectrumSection(SpectrixAudioProcessor &p)
        : audioProcessor(p),
          spectrumDisplay(audioProcessor.spectralCompressor, audioProcessor.getSampleRate()),
          gainReductionVisualizer(audioProcessor.spectralCompressor,
                                  audioProcessor.getSampleRate()),
          responseCurve(audioProcessor.responseCurve, audioProcessor.getSampleRate()),
          grid(audioProcessor.getSampleRate()) {
        // Group box
        // addAndMakeVisible(grid);
        addAndMakeVisible(spectrumDisplay);
        addAndMakeVisible(gainReductionVisualizer);
        addAndMakeVisible(responseCurve);
    }

    void resized() override {
        auto bounds = getLocalBounds();

        // grid.setBounds(bounds);
        spectrumDisplay.setBounds(bounds);
        responseCurve.setBounds(bounds);
        gainReductionVisualizer.setBounds(bounds);
    }

  private:
    SpectrixAudioProcessor &audioProcessor;
    SpectrumDisplay<Parameters::FFT_SIZE> spectrumDisplay;
    SpectralGainReductionVisualizer<Parameters::FFT_SIZE> gainReductionVisualizer;
    ResponseCurve responseCurve;
    SpectrumGrid grid;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumSection)
};
