#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include <cstdio>
#include <array>
#include <juce_dsp/juce_dsp.h>
#include "CircularBuffer.h"

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2> class SpectralProcessor {
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

            magnitudes.fill(0.0f);
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

      const std::array<float, FFT_SIZE / 2 + 1> &getMagnitudes() const { return magnitudes; }

      size_t getFFTSize() const { return FFT_SIZE; }

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

            fftBuffer.fill(0.0f);

            for(size_t i = 0; i < FFT_SIZE; ++i)
                  fftBuffer[i] = inFifo[i];

            window.multiplyWithWindowingTable(fftBuffer.data(), FFT_SIZE);
            fft.performRealOnlyForwardTransform(fftBuffer.data());
            storeMagnitudes(fftBuffer, channel);
            processFFTBins(fftBuffer);
            fft.performRealOnlyInverseTransform(fftBuffer.data());
            window.multiplyWithWindowingTable(fftBuffer.data(), FFT_SIZE);

            for(size_t i = 0; i < FFT_SIZE; ++i)
                  fftBuffer[i] += olaBuffer[i];

            for(size_t i = 0; i < hopSize; ++i)
                  outFifo.push(fftBuffer[i]);

            std::copy(fftBuffer.begin() + hopSize, fftBuffer.begin() + FFT_SIZE, olaBuffer.begin());
            std::fill(olaBuffer.begin() + (FFT_SIZE - hopSize), olaBuffer.end(), 0.0f);

            for(size_t i = 0; i < hopSize; ++i)
                  inFifo.pop();
      }

      void storeMagnitudes(const std::array<float, FFT_SIZE * 2> &fftBuffer, int channel) {
            juce::SpinLock::ScopedLockType lock(mutex);

            const float windowCompensation = 2.0f; // Hann window compensation
            const float scale = (2.0f / FFT_SIZE) * windowCompensation;
            const float dcNyquistScale = (1.0f / FFT_SIZE) * windowCompensation;

            const float channelWeight = (NUM_CHANNELS > 1) ? 0.5f : 1.0f;

            if(channel == 0) {
                  tempMagnitudes[0] = std::abs(fftBuffer[0]) * dcNyquistScale * channelWeight;

                  for(size_t i = 1; i < FFT_SIZE / 2; ++i) {
                        float real = fftBuffer[i];
                        float imag = fftBuffer[i + FFT_SIZE];
                        tempMagnitudes[i]
                         = std::sqrt(real * real + imag * imag) * scale * channelWeight;
                  }

                  tempMagnitudes[FFT_SIZE / 2]
                   = std::abs(fftBuffer[FFT_SIZE / 2]) * dcNyquistScale * channelWeight;

                  if(NUM_CHANNELS == 1) {
                        magnitudes = tempMagnitudes;
                  }
            } else if(channel == 1 && NUM_CHANNELS > 1) {
                  magnitudes[0]
                   = tempMagnitudes[0] + (std::abs(fftBuffer[0]) * dcNyquistScale * channelWeight);

                  for(size_t i = 1; i < FFT_SIZE / 2; ++i) {
                        float real = fftBuffer[i];
                        float imag = fftBuffer[i + FFT_SIZE];
                        float mag = std::sqrt(real * real + imag * imag) * scale * channelWeight;
                        magnitudes[i] = tempMagnitudes[i] + mag;
                  }

                  magnitudes[FFT_SIZE / 2]
                   = tempMagnitudes[FFT_SIZE / 2]
                     + (std::abs(fftBuffer[FFT_SIZE / 2]) * dcNyquistScale * channelWeight);
            }
      }

      // Member variables
      const size_t hopSize;
      juce::dsp::WindowingFunction<float> window;
      juce::dsp::FFT fft;

      std::array<CircularBuffer<float, FFT_SIZE * 2>, NUM_CHANNELS> inputFifos;
      std::array<CircularBuffer<float, FFT_SIZE * 2>, NUM_CHANNELS> outputFifos;
      std::array<std::array<float, FFT_SIZE>, NUM_CHANNELS> OLABuffers;
      std::array<std::array<float, FFT_SIZE * 2>, NUM_CHANNELS> fftBuffers;

      mutable juce::SpinLock mutex;
      std::array<float, FFT_SIZE / 2 + 1> magnitudes;     // Final averaged magnitudes for display
      std::array<float, FFT_SIZE / 2 + 1> tempMagnitudes; // Temporary storage for stereo averaging

    protected:
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralProcessor)
};

// #########################
// #                       #
// #  SPECTRAL COMPRESSOR  #
// #                       #
// #########################

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2>
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
