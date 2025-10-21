#pragma once

#include "FFTProcessor.h"
#include "GaussianResponseCurve.h"
#include "PluginParameters.h"
#include <JuceHeader.h>
#include <array>
#include <cstddef>
#include <vector>
#include <cmath>

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2>
class SpectralCompressor : public FFTProcessor<FFT_SIZE, NUM_CHANNELS> {
  public:
    SpectralCompressor(GaussianResponseCurve &responseCurveReference)
        : FFTProcessor<FFT_SIZE, NUM_CHANNELS>(), responseCurve(responseCurveReference) {}

    std::array<float, (size_t)FFT_SIZE / 2 + 1> getGainReductionArray() const {
        return gainReductionArray;
    }

  private:
    void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {
        auto gaussianPeaks = responseCurve.getGaussianPeaks();
        if(gaussianPeaks.empty())
            return;

        const float nyquist = static_cast<float>(this->sampleRate) * 0.5f;

        // Match the scaling from your storeMagnitudes function
        constexpr float windowComp = 0.49f;
        const float scale = (2.0f / FFT_SIZE) * windowComp;
        const float dcNyquistScale = (1.0f / FFT_SIZE) * windowComp;

        for(size_t bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            float frequency
             = (static_cast<float>(bin) / FFT_SIZE) * static_cast<float>(this->sampleRate);

            if(frequency < 20.0f || frequency > nyquist)
                continue;

            // Extract magnitude and phase
            float magnitude, phase;
            if(bin == 0 || bin == FFT_SIZE / 2) {
                magnitude = std::abs(transformedBuffer[bin]) * dcNyquistScale;
                phase = (transformedBuffer[bin] >= 0.0f) ? 0.0f : juce::MathConstants<float>::pi;
            } else {
                float real = transformedBuffer[bin];
                float imag = transformedBuffer[bin + FFT_SIZE];
                magnitude = std::sqrt(real * real + imag * imag) * scale;
                phase = std::atan2(imag, real);
            }

            // Convert to dB (matching your updateMagnitudes conversion)
            float magnitudeDB = (magnitude > 1e-10f) ? 20.0f * std::log10(magnitude) : -100.0f;

            // Get threshold from Gaussian response curve
            float thresholdDB = calculateGaussianSum(frequency, gaussianPeaks);

            // Hard clip if above threshold
            if(magnitudeDB > thresholdDB) {
                float newMagnitudeDB = thresholdDB;
                magnitude = std::pow(10.0f, newMagnitudeDB / 20.0f);
                gainReductionArray[bin] = magnitudeDB - newMagnitudeDB;
            } else {
                gainReductionArray[bin] = 0.0f;
            }

            // Undo the scaling before writing back
            if(bin == 0 || bin == FFT_SIZE / 2) {
                magnitude /= dcNyquistScale;
                transformedBuffer[bin] = magnitude * std::cos(phase);
            } else {
                magnitude /= scale;
                transformedBuffer[bin] = magnitude * std::cos(phase);
                transformedBuffer[bin + FFT_SIZE] = magnitude * std::sin(phase);
            }
        }
    }

    float calculateGaussianSum(float frequency, const std::vector<GaussianPeak> &peaks) {
        responseCurveShiftDB = responseCurve.getResponseCurveShiftDB();
        // Work in log-frequency space (same as ResponseCurve)
        float logFreq = std::log10(frequency);
        float sumDB = 0.0f;

        for(const auto &peak : peaks) {
            float peakLogFreq = std::log10(peak.frequency);
            float dx = logFreq - peakLogFreq;

            // Gaussian in log-frequency space
            float exponent = -0.5f * (dx * dx) / (peak.sigmaNorm * peak.sigmaNorm);
            float gaussianValue = std::exp(exponent);

            // Sum in dB (linear addition in dB space)
            sumDB += peak.gainDB * gaussianValue;
        }

        return sumDB + responseCurveShiftDB;
    }

    std::array<float, (size_t)FFT_SIZE / 2 + 1> gainReductionArray{};

    GaussianResponseCurve &responseCurve;
    mutable juce::SpinLock thresholdMutex;
    float responseCurveShiftDB = Parameters::responseCurveShiftDB;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralCompressor)
};
