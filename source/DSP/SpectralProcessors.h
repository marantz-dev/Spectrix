
#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include <cstdio>
#include <array>
#include <juce_dsp/juce_dsp.h>
#include "CircularBuffer.h"

template <size_t FFT_SIZE, size_t NUM_CHANNELS = 2> class SpectralProcessor {
    public:
      SpectralProcessor()
          : hopSize(FFT_SIZE / 2),
            window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann, false),
            fft((int)log2(FFT_SIZE)) {
            // ########## CONSTRUCTOR ##########
            for(auto &buf : OLABuffers)
                  buf.fill(0.0f);

            for(auto &buf : fftBuffers)
                  buf.fill(0.0f);
      }

      ~SpectralProcessor() = default;

      void processBlock(juce::AudioBuffer<float> &buffer) {
            const int numSamples = buffer.getNumSamples();
            const int numChannels = buffer.getNumChannels();

            jassert(numChannels <= NUM_CHANNELS);

            for(int ch = 0; ch < numChannels; ++ch) {
                  auto *data = buffer.getWritePointer(ch);

                  for(int i = 0; i < numSamples; ++i)
                        inputFifos[ch].push(data[i]);

                  computeFFT(ch);

                  for(int i = 0; i < numSamples; ++i)
                        data[i] = outputFifos[ch].pop();
            }
      }

      virtual void processFFTBins(std::array<float, FFT_SIZE * 2> &fftBuffer) {
            // ##########################
            // #                        #
            // #  SPECTRAL PROCESSING!  #
            // #                        #
            // ##########################
      }

    private:
      void computeFFT(int channel) {
            jassert(channel < NUM_CHANNELS);
            auto &inFifo = inputFifos[channel];
            auto &outFifo = outputFifos[channel];
            auto &fftBuffer = fftBuffers[channel];
            auto &olaBuffer = OLABuffers[channel];

            while(inFifo.size() >= FFT_SIZE) {
                  for(size_t i = 0; i < FFT_SIZE; ++i)
                        fftBuffer[i] = inFifo[i];

                  // ########## COMPUTE FFT ##########

                  window.multiplyWithWindowingTable(fftBuffer.data(), FFT_SIZE);
                  fft.performRealOnlyForwardTransform(fftBuffer.data());
                  processFFTBins(fftBuffer);
                  fft.performRealOnlyInverseTransform(fftBuffer.data());

                  // ########## Overlap-Add ##########
                  for(size_t i = 0; i < FFT_SIZE; ++i) {
                        fftBuffer[i] += olaBuffer[i];
                  }
                  for(size_t i = 0; i < hopSize; ++i) {
                        outFifo.push(fftBuffer[i]);
                  }

                  std::copy(fftBuffer.begin() + hopSize, fftBuffer.begin() + FFT_SIZE,
                            olaBuffer.begin());
                  std::fill(olaBuffer.begin() + (FFT_SIZE - hopSize), olaBuffer.end(), 0.0f);

                  for(size_t i = 0; i < hopSize; ++i)
                        inFifo.pop();
            }
      }

      // ######################
      // #                    #
      // #  MEMBER VARIABLES  #
      // #                    #
      // ######################

      const size_t hopSize;

      juce::dsp::WindowingFunction<float> window;
      juce::dsp::FFT fft;

      std::array<CircularBuffer<float, FFT_SIZE * 2>, NUM_CHANNELS> inputFifos;
      std::array<CircularBuffer<float, FFT_SIZE * 2>, NUM_CHANNELS> outputFifos;
      std::array<std::array<float, FFT_SIZE>, NUM_CHANNELS> OLABuffers;

    protected:
      std::array<std::array<float, FFT_SIZE * 2>, NUM_CHANNELS> fftBuffers;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralProcessor)
};
