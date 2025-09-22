
#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include <cstdio>
#include <array>
#include <juce_dsp/juce_dsp.h>
#include "CircularBuffer.h"

// ################################
// #                              #
// #  VIRTUAL SPECTRAL PROCESSOR  #
// #                              #
// ################################

template <size_t FFT_SIZE, size_t NUM_CHANNELS = 2> class SpectralProcessor {
    public:
      SpectralProcessor()
          : hopSize(FFT_SIZE / 2),
            window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann, false),
            fft((int)log2(FFT_SIZE)) {
            static_assert((FFT_SIZE & (FFT_SIZE - 1)) == 0, "FFT_SIZE must be a power of 2");
            static_assert(FFT_SIZE >= 64, "FFT_SIZE must be at least 64");
            static_assert(NUM_CHANNELS > 0 && NUM_CHANNELS <= 8,
                          "NUM_CHANNELS must be between 1 and 8");
      }

      ~SpectralProcessor() = default;

      void prepareToPlay() {
            for(auto &buf : OLABuffers)
                  buf.fill(0.0f);

            for(auto &buf : fftBuffers)
                  buf.fill(0.0f);

            for(auto &fifo : inputFifos)
                  fifo.clear();

            for(auto &fifo : outputFifos)
                  fifo.clear();
      }

      void processBlock(juce::AudioBuffer<float> &buffer) {
            const int numSamples = buffer.getNumSamples();
            const int numChannels = buffer.getNumChannels();

            jassert(numChannels <= NUM_CHANNELS);

            for(int ch = 0; ch < numChannels; ++ch) {
                  auto *data = buffer.getWritePointer(ch);

                  for(int i = 0; i < numSamples; ++i) {
                        inputFifos[ch].push(data[i]);
                        computeFFT(ch);
                        data[i] = outputFifos[ch].pop();
                  }
            }
      }

      virtual void processFFTBins(std::array<float, FFT_SIZE * 2> &fftBuffer) = 0;

    private:
      void computeFFT(int channel) {
            if(inputFifos[channel].size() < FFT_SIZE)
                  return;

            jassert(channel < NUM_CHANNELS);
            auto &inFifo = inputFifos[channel];
            auto &outFifo = outputFifos[channel];
            auto &fftBuffer = fftBuffers[channel];
            auto &olaBuffer = OLABuffers[channel];

            for(size_t i = 0; i < FFT_SIZE; ++i)
                  fftBuffer[i] = inFifo[i];

            window.multiplyWithWindowingTable(fftBuffer.data(), FFT_SIZE);
            fft.performRealOnlyForwardTransform(fftBuffer.data());
            processFFTBins(fftBuffer);
            fft.performRealOnlyInverseTransform(fftBuffer.data());

            for(size_t i = 0; i < FFT_SIZE; ++i)
                  fftBuffer[i] += olaBuffer[i];
            for(size_t i = 0; i < hopSize; ++i)
                  outFifo.push(fftBuffer[i]);

            std::copy(fftBuffer.begin() + hopSize, fftBuffer.begin() + FFT_SIZE, olaBuffer.begin());
            std::fill(olaBuffer.begin() + (FFT_SIZE - hopSize), olaBuffer.end(), 0.0f);

            for(size_t i = 0; i < hopSize; ++i)
                  inFifo.pop();
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
      std::array<std::array<float, FFT_SIZE * 2>, NUM_CHANNELS> fftBuffers;

    protected:
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralProcessor)
};

// #########################
// #                       #
// #  SPECTRAL COMPRESSOR  #
// #                       #
// #########################

template <size_t FFT_SIZE, size_t NUM_CHANNELS = 2>
class SpectralCompressor : public SpectralProcessor<FFT_SIZE, NUM_CHANNELS> {
    public:
      SpectralCompressor() : SpectralProcessor<FFT_SIZE, NUM_CHANNELS>() {}
      void setGain(float newGain) { gain = newGain; }

    private:
      void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {
            // ####################################
            // #                                  #
            // #  SPECTRAL PROCESSING GOES HERE!  #
            // #                                  #
            // ####################################

            // Example: Simple "Spectral" gain reduction
            for(size_t i = 0; i < FFT_SIZE; ++i) {
                  float real = transformedBuffer[i * 2];
                  float imag = transformedBuffer[i * 2 + 1];
                  float magnitude = std::sqrt(real * real + imag * imag);
                  float phase = std::atan2(imag, real);

                  // Apply gain reduction
                  magnitude *= gain;

                  // Reconstruct the complex number
                  transformedBuffer[i * 2] = magnitude * std::cos(phase);
                  transformedBuffer[i * 2 + 1] = magnitude * std::sin(phase);

                  // Assolutamente inutile Rimuovere sta menata e se vuoi scalare scala direttamente
                  // real e img dio spennato
            }
      }

      float gain = 1.0f;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralCompressor)
};
