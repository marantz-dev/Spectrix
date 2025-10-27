

#pragma once
#include <JuceHeader.h>
#include "GainReductionVisualizer.h"
#include "PluginParameters.h"
#include "PluginProcessor.h"
#include "ResponseCurve.h"
#include "Spectrum.h"
#include "SpectrumGrid.h"
#include "juce_graphics/juce_graphics.h"

class SpectrumSection : public juce::Component {
  public:
    SpectrumSection(SpectrixAudioProcessor &p)
        : audioProcessor(p), spectrumDisplay(audioProcessor.spectralCompressor,
                                             audioProcessor.getSampleRate(), juce::Colours::cyan),
          gainReductionVisualizer(audioProcessor.spectralCompressor,
                                  audioProcessor.getSampleRate()),
          responseCurve(audioProcessor.responseCurve, audioProcessor.getSampleRate()),
          grid(audioProcessor.getSampleRate()),
          drySpectrum(audioProcessor.spectralVisualizer, audioProcessor.getSampleRate(),
                      juce::Colours::blueviolet.darker(3)) {
        // Group box
        addAndMakeVisible(grid);
        addAndMakeVisible(drySpectrum);
        addAndMakeVisible(spectrumDisplay);
        addAndMakeVisible(gainReductionVisualizer);
        addAndMakeVisible(responseCurve);
    }

    void resized() override {
        auto bounds = getLocalBounds();

        grid.setBounds(bounds);
        drySpectrum.setBounds(bounds);
        spectrumDisplay.setBounds(bounds);
        responseCurve.setBounds(bounds);
        gainReductionVisualizer.setBounds(bounds);
    }

    void prepareToPlay(double newSampleRate) {
        // spectrumDisplay.setSampleRate(newSampleRate);
        // gainReductionVisualizer.setSampleRate(newSampleRate);
        // responseCurve.setSampleRate(newSampleRate);
    }

  private:
    SpectrixAudioProcessor &audioProcessor;
    SpectrumDisplay<Parameters::FFT_SIZE> spectrumDisplay;
    SpectralGainReductionVisualizer<Parameters::FFT_SIZE> gainReductionVisualizer;
    SpectrumDisplay<Parameters::FFT_SIZE> drySpectrum;
    ResponseCurve responseCurve;
    SpectrumGrid grid;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumSection)
};
