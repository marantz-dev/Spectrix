#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <array>
#include <juce_dsp/juce_dsp.h>
#include "CircularBuffer.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2> class SpectralProcessor {
  public:
    SpectralProcessor()
        : hopSize(FFT_SIZE / 2), window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann, false),
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
        processFFTBins(fftBuffer);
        storeMagnitudes(fftBuffer, channel);
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

    void storeMagnitudes(const std::array<float, FFT_SIZE * 2> &fftBuffer, int channel) {
        juce::SpinLock::ScopedLockType lock(mutex);

        constexpr float windowCompensation = 2.0f; // Hann window compensation
        const float scale = (2.0f / FFT_SIZE) * windowCompensation;
        const float dcNyquistScale = (1.0f / FFT_SIZE) * windowCompensation;
        const float channelWeight = (NUM_CHANNELS > 1) ? 0.5f : 1.0f;

        auto computeMag = [&](size_t bin) {
            if(bin == 0 || bin == FFT_SIZE / 2) {
                return std::abs(fftBuffer[bin]) * dcNyquistScale * channelWeight;
            } else {
                float real = fftBuffer[bin];
                float imag = fftBuffer[bin + FFT_SIZE];
                return std::sqrt(real * real + imag * imag) * scale * channelWeight;
            }
        };

        auto &target = (channel == 0 ? tempMagnitudes : magnitudes);

        for(size_t i = 0; i <= FFT_SIZE / 2; ++i) {
            float mag = computeMag(i);
            if(channel == 0) {
                tempMagnitudes[i] = mag;
            } else if(NUM_CHANNELS > 1) {
                target[i] = tempMagnitudes[i] + mag;
            }
        }

        if(channel == 0 && NUM_CHANNELS == 1) {
            magnitudes = tempMagnitudes;
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
    std::array<float, FFT_SIZE / 2 + 1> magnitudes;
    std::array<float, FFT_SIZE / 2 + 1> tempMagnitudes;

  protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralProcessor)
};

// #################################################
// #                                               #
// #  FOR TESTING PURPOSES WE IMPLEMENT A CLIPPER  #
// #                                               #
// #################################################

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2>
class SpectralClipper : public SpectralProcessor<FFT_SIZE, NUM_CHANNELS> {
    // RICORDA!!!!!!: QUESTO CLIPPER E UNA BOIATA GIUTO PER TESTARE MA E TUTTO SBAGLIATO
  public:
    SpectralClipper() : SpectralProcessor<FFT_SIZE, NUM_CHANNELS>() {}

    void setThreshold(float newThreshold) {
        threshold = newThreshold * FFT_SIZE / 4;
        // DBG("New THR: " << threshold);
    }

  private:
    void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {
        for(size_t i = 1; i < FFT_SIZE; ++i) {
            float real = transformedBuffer[i * 2];
            float imag = transformedBuffer[i * 2 + 1];
            float mag = std::sqrt(real * real + imag * imag);
            float phase = std::atan2(imag, real);
            // Hard clip
            if(mag > threshold)
                mag = threshold;

            // Reconstruct complex bin
            transformedBuffer[i * 2] = mag * std::cos(phase);
            transformedBuffer[i * 2 + 1] = mag * std::sin(phase);
        }
    }

    float threshold = 1.0f; // linear amplitude scale

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralClipper)
};
