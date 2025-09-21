
#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <juce_dsp/juce_dsp.h>

class SpectralProcessor {
    public:
      SpectralProcessor(int order = 10)
          : fftSize(1 << order), fft(order), hopSize(fftSize / 2),
            window(fftSize, juce::dsp::WindowingFunction<float>::hann) {
            fftBuffer.resize(fftSize * 2, 0.0f);
            olaBuffer.resize(fftSize, 0.0f);
      }

      ~SpectralProcessor() = default;

      void processBlock(AudioBuffer<float> &mainBuffer) {
            const auto numSamples = mainBuffer.getNumSamples();
            const auto numChannels = mainBuffer.getNumChannels();
            auto data = mainBuffer.getArrayOfWritePointers();
            for(int i = 0; i < numSamples; ++i) {
                  pushNextSampleIntoInputFifo(data[0][i]);
                  computeFFT();
                  data[0][i] = pullSampleFromOutputFifo();
            }
            for(int ch = 1; ch < numChannels; ++ch) {}
      }
      size_t getFFTSize() const { return fftSize; }

    protected:
      virtual void processSpectrum() {}

      void pushNextSampleIntoInputFifo(float sample) { inputFifo.push_back(sample); }

      std::vector<float> fftBuffer;

    private:
      float pullSampleFromOutputFifo() {
            if(!outputFifo.empty()) {
                  float sample = outputFifo.front();
                  outputFifo.pop_front();
                  return sample;
            } else {
                  return 0.0f; // Return silence if no samples are available
            }
      }

      void computeFFT() {
            if(inputFifo.size() >= fftSize) {
                  for(size_t i = 0; i < fftSize; ++i)
                        fftBuffer[i] = inputFifo[i];

                  window.multiplyWithWindowingTable(fftBuffer.data(), fftSize);
                  fft.performRealOnlyForwardTransform(fftBuffer.data());
                  processSpectrum();
                  fft.performRealOnlyInverseTransform(fftBuffer.data());
                  overlapAdd();

                  for(size_t i = 0; i < hopSize; ++i)
                        inputFifo.pop_front();
            }
      }
      void overlapAdd() {
            for(size_t i = 0; i < fftSize; ++i)
                  fftBuffer[i] += olaBuffer[i];

            for(size_t i = 0; i < hopSize; ++i)
                  outputFifo.push_back(fftBuffer[i]);

            for(size_t i = 0; i < fftSize - hopSize; ++i)
                  olaBuffer[i] = fftBuffer[i + hopSize];

            for(size_t i = fftSize - hopSize; i < fftSize; ++i)
                  olaBuffer[i] = 0.0f;
      }
      size_t fftSize;
      size_t hopSize;
      std::vector<float> olaBuffer;
      juce::dsp::WindowingFunction<float> window;
      juce::dsp::FFT fft;

      std::deque<float> inputFifo;
      std::deque<float> outputFifo;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralProcessor)
};

class SpectralClip : public SpectralProcessor {
    public:
      SpectralClip() : SpectralProcessor() {}
      ~SpectralClip() {}
};
