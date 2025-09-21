
#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <juce_dsp/juce_dsp.h>

class SpectralProcessor {
    public:
      SpectralProcessor(int order = 10)
          : fftSize(1 << order), fft(order), hopSize(fftSize / 4),
            window(fftSize, juce::dsp::WindowingFunction<float>::hann) {
            fftBuffer.resize(fftSize * 2, 0.0f);
            olaBuffer.resize(fftSize, 0.0f);
      }
      ~SpectralProcessor() {}
      void prepareToPlay() {}

      void processBlock(AudioBuffer<float> &mainBuffer) {
            const auto numSamples = mainBuffer.getNumSamples();
            auto data = mainBuffer.getArrayOfWritePointers();
            for(int i = 0; i < numSamples; ++i) {
                  pushNextSampleIntoInputFifo(data[0][i]);
                  computeFFT();
                  data[0][i] = pullSampleFromOutputFifo();
            }
      }
      virtual void processSpectrum() {}

    protected:
      void pushNextSampleIntoInputFifo(float sample) { inputFifo.push_back(sample); }

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
                  window.multiplyWithWindowingTable(fftBuffer.data(), fftSize);

                  for(size_t i = 0; i < fftSize; ++i)
                        fftBuffer[i] += olaBuffer[i]; // add leftover from previous frame

                  // Copy first hopSize samples to output FIFO
                  for(size_t i = 0; i < hopSize; ++i)
                        outputFifo.push_back(fftBuffer[i]);

                  // Shift remaining samples into OLA buffer
                  for(size_t i = 0; i < fftSize - hopSize; ++i)
                        olaBuffer[i] = fftBuffer[i + hopSize];

                  // Zero out the rest of OLA buffer
                  for(size_t i = fftSize - hopSize; i < fftSize; ++i)
                        olaBuffer[i] = 0.0f;

                  // Remove hopSize samples from inputFifo
                  for(size_t i = 0; i < hopSize; ++i)
                        inputFifo.pop_front();
            }
      }

      double sampleRate;

      size_t fftSize;
      juce::dsp::FFT fft;
      std::vector<float> fftBuffer;

      int hopSize;
      juce::dsp::WindowingFunction<float> window;
      std::vector<float> olaBuffer;

      std::deque<float> inputFifo;
      std::deque<float> outputFifo;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralProcessor)
};

class SpectralClip : public SpectralProcessor {
    public:
      SpectralClip(int order = 10) : SpectralProcessor(order) {}
      ~SpectralClip() {}
};
