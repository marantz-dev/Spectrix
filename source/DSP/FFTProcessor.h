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

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2> class FFTProcessor {
  public:
    FFTProcessor()
        : hopSize(FFT_SIZE / 4),
          window(FFT_SIZE, juce::dsp::WindowingFunction<float>::blackmanHarris, false),
          fft((int)log2(FFT_SIZE)) {
        static_assert((FFT_SIZE & (FFT_SIZE - 1)) == 0, "FFT_SIZE must be a power of 2");
        static_assert(FFT_SIZE >= 64, "FFT_SIZE must be at least 64");
        static_assert(NUM_CHANNELS > 0 && NUM_CHANNELS <= 8,
                      "NUM_CHANNELS must be between 1 and 8");
        computeWindowCompensation();
    }

    ~FFTProcessor() = default;

    void virtual prepareToPlay(double sampleRate) {
        for(auto &buf : OLABuffers)
            buf.fill(0.0f);

        for(auto &buf : fftBuffers)
            buf.fill(0.0f);

        for(auto &fifo : inputFifos)
            fifo.clear();

        for(auto &fifo : outputFifos)
            fifo.clear();

        magnitudes.fill(0.0f);
        this->sampleRate = sampleRate;
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

    double getSampleRate() const { return sampleRate; }

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
        window.multiplyWithWindowingTable(fftBuffer.data(), FFT_SIZE);

        for(size_t i = 0; i < FFT_SIZE; ++i)
            fftBuffer[i] *= windowCompensation[i];

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

        constexpr float windowComp = 0.49f; // Hann window compensation
        const float scale = (2.0f / FFT_SIZE) * windowComp;
        const float dcNyquistScale = (1.0f / FFT_SIZE) * windowComp;
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

    void computeWindowCompensation() {
        std::array<float, FFT_SIZE * 2> overlapSum;
        overlapSum.fill(0.0f);

        std::array<float, FFT_SIZE> tempWindow;
        std::fill(tempWindow.begin(), tempWindow.end(), 1.0f);
        window.multiplyWithWindowingTable(tempWindow.data(), FFT_SIZE);

        for(size_t i = 0; i < FFT_SIZE; ++i) {
            tempWindow[i] = tempWindow[i] * tempWindow[i];
        }

        const size_t numFrames = 8; // Extra frames to ensure steady state

        for(size_t frame = 0; frame < numFrames; ++frame) {
            size_t offset = frame * hopSize;
            for(size_t i = 0; i < FFT_SIZE; ++i) {
                if(offset + i < overlapSum.size()) {
                    overlapSum[offset + i] += tempWindow[i];
                }
            }
        }

        size_t steadyStateStart = FFT_SIZE;

        for(size_t i = 0; i < FFT_SIZE; ++i) {
            float sum = overlapSum[steadyStateStart + i];
            if(sum > 1e-6f) {
                windowCompensation[i] = 1.0f / sum;
            } else {
                windowCompensation[i] = 1.0f; // Fallback
            }
        }
    }

    const size_t hopSize;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, FFT_SIZE> windowCompensation;
    juce::dsp::FFT fft;

    std::array<CircularBuffer<float, FFT_SIZE * 2>, NUM_CHANNELS> inputFifos;
    std::array<CircularBuffer<float, FFT_SIZE * 2>, NUM_CHANNELS> outputFifos;
    std::array<std::array<float, FFT_SIZE>, NUM_CHANNELS> OLABuffers;
    std::array<std::array<float, FFT_SIZE * 2>, NUM_CHANNELS> fftBuffers;

    mutable juce::SpinLock mutex;
    std::array<float, FFT_SIZE / 2 + 1> magnitudes;
    std::array<float, FFT_SIZE / 2 + 1> tempMagnitudes;

  protected:
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessor)
};
