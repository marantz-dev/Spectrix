#pragma once
#include "FFTProcessor.h"
#include "SpectralCompressor.h"
#include <JuceHeader.h>
#include "PluginParameters.h"
#include "GaussianResponseCurve.h"
#include "SpectralVisualizer.h"

class SpectrixAudioProcessor : public juce::AudioProcessor,
                               public AudioProcessorValueTreeState::Listener {
  public:
    //==============================================================================
    SpectrixAudioProcessor();
    ~SpectrixAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }

    bool producesMidi() const override { return false; }

    bool isMidiEffect() const override { return false; }

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }

    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int index) override {}

    const juce::String getProgramName(int index) override { return {}; }

    void changeProgramName(int index, const juce::String &newName) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    void
    updateProbe(juce::Atomic<float> &probe, const juce::AudioBuffer<float> &buf, int numSamples) {
        probe.set(jmax(buf.getMagnitude(0, numSamples), probe.get()));
    }

    GaussianResponseCurve responseCurve;
    SpectralCompressor<Parameters::FFT_SIZE> spectralCompressor;
    SpectralVisualizer<Parameters::FFT_SIZE> spectralVisualizer;

    SmoothedValue<float, ValueSmoothingTypes::Linear> inputGain, outputGain;
    Atomic<float> inputProbe;
    Atomic<float> outputProbe;

  private:
    void parameterChanged(const String &paramID, float newValue) override;

    AudioProcessorValueTreeState parameters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrixAudioProcessor)
};
