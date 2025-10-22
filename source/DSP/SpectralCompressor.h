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
        : FFTProcessor<FFT_SIZE, NUM_CHANNELS>(), responseCurve(responseCurveReference) {
        // Initialize envelope followers to 0 dB (no gain reduction)
        envelopeFollowers.fill(0.0f);
    }

    void setClipperActive(bool active) { clipperActive = active; }

    bool isClipperActive() const { return clipperActive; }

    std::array<float, (size_t)FFT_SIZE / 2 + 1> getGainReductionArray() const {
        return gainReductionArray;
    }

    void setAttackTime(float timeMs) {
        attackTimeMs = timeMs;
        updateCoefficients();
    }

    void setReleaseTime(float timeMs) {
        releaseTimeMs = timeMs;
        updateCoefficients();
    }

    void setRatio(float newRatio) { ratio = juce::jlimit(1.0f, 20.0f, newRatio); }

    void setKnee(float kneeDB) { kneeWidthDB = juce::jlimit(0.0f, 12.0f, kneeDB); }

    void prepareToPlay(double newSampleRate) override {
        FFTProcessor<FFT_SIZE, NUM_CHANNELS>::prepareToPlay(newSampleRate);
        this->nyquist = static_cast<float>(newSampleRate) * 0.5f;
        this->windowComp = 0.49f;
        this->scale = (2.0f / FFT_SIZE) * windowComp;
        this->dcNyquistScale = (1.0f / FFT_SIZE) * windowComp;

        updateCoefficients();
        envelopeFollowers.fill(0.0f);
    }

  private:
    void updateCoefficients() {
        if(this->sampleRate <= 0.0)
            return;

        // Calculate time per FFT hop (overlap assumed to be 4x)
        float hopSize = FFT_SIZE / 4.0f;
        float timePerHop = hopSize / static_cast<float>(this->sampleRate);

        // Convert attack/release times to coefficients
        // Using exponential smoothing: coeff = exp(-1 / (time / hopTime))
        attackCoeff = std::exp(-timePerHop / (attackTimeMs * 0.001f));
        releaseCoeff = std::exp(-timePerHop / (releaseTimeMs * 0.001f));

        // Clamp coefficients to valid range
        attackCoeff = juce::jlimit(0.0f, 0.999f, attackCoeff);
        releaseCoeff = juce::jlimit(0.0f, 0.999f, releaseCoeff);
    }

    void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {
        auto gaussianPeaks = responseCurve.getGaussianPeaks();
        if(gaussianPeaks.empty())
            return;

        for(size_t bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            float frequency = bin / static_cast<float>(FFT_SIZE) * this->sampleRate;
            float magnitude, phase;

            // Extract magnitude and phase
            if(bin == 0 || bin == FFT_SIZE / 2) {
                magnitude = std::abs(transformedBuffer[bin]) * dcNyquistScale;
                phase = (transformedBuffer[bin] >= 0.0f) ? 0.0f : juce::MathConstants<float>::pi;
            } else {
                float real = transformedBuffer[bin];
                float imag = transformedBuffer[bin + FFT_SIZE];
                magnitude = std::sqrt(real * real + imag * imag) * scale;
                phase = std::atan2(imag, real);
            }

            // Convert to dB
            float magnitudeDB = (magnitude > 1e-10f) ? 20.0f * std::log10(magnitude) : -100.0f;

            // Get dynamic threshold from Gaussian curve
            float thresholdDB = calculateGaussianSum(frequency, gaussianPeaks);

            float gainReductionDB;

            if(clipperActive) {
                // Hard clipping behavior - instant hard limiting at threshold
                if(magnitudeDB > thresholdDB) {
                    gainReductionDB = magnitudeDB - thresholdDB;
                } else {
                    gainReductionDB = 0.0f;
                }
            } else {
                // Normal compressor behavior with ratio and knee
                gainReductionDB = calculateGainReduction(magnitudeDB, thresholdDB);
            }

            // Apply envelope follower for smoothing (only for compressor mode)
            float smoothedGainReductionDB;
            if(clipperActive) {
                // No smoothing for clipper - instant response
                smoothedGainReductionDB = gainReductionDB;
            } else {
                smoothedGainReductionDB = applyEnvelopeFollower(bin, gainReductionDB);
            }

            // Store for visualization
            gainReductionArray[bin] = smoothedGainReductionDB;

            // Apply gain reduction
            float gainLinear = std::pow(10.0f, -smoothedGainReductionDB / 20.0f);
            magnitude *= gainLinear;

            // Reconstruct complex values
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

    float calculateGainReduction(float inputDB, float thresholdDB) {
        float overThresholdDB = inputDB - thresholdDB;

        if(overThresholdDB <= -kneeWidthDB / 2.0f) {
            // Below knee - no compression
            return 0.0f;
        } else if(overThresholdDB >= kneeWidthDB / 2.0f) {
            // Above knee - full compression
            return overThresholdDB * (1.0f - 1.0f / ratio);
        } else {
            // In knee region - soft knee
            float x = overThresholdDB + kneeWidthDB / 2.0f;
            float kneeGain = (x * x) / (2.0f * kneeWidthDB);
            return kneeGain * (1.0f - 1.0f / ratio);
        }
    }

    float applyEnvelopeFollower(size_t bin, float targetGainReductionDB) {
        float currentEnvelope = envelopeFollowers[bin];

        // Choose coefficient based on whether we're attacking or releasing
        float coeff = (targetGainReductionDB > currentEnvelope) ? attackCoeff : releaseCoeff;

        // Exponential smoothing
        float newEnvelope = coeff * currentEnvelope + (1.0f - coeff) * targetGainReductionDB;

        envelopeFollowers[bin] = newEnvelope;
        return newEnvelope;
    }

    float calculateGaussianSum(float frequency, const std::vector<GaussianPeak> &peaks) {
        responseCurveShiftDB = responseCurve.getResponseCurveShiftDB();
        float logFreq = std::log10(frequency);
        float sumDB = 0.0f;

        for(const auto &peak : peaks) {
            float peakLogFreq = std::log10(peak.frequency);
            float dx = logFreq - peakLogFreq;
            float exponent = -0.5f * (dx * dx) / (peak.sigmaNorm * peak.sigmaNorm);
            float gaussianValue = std::exp(exponent);
            sumDB += peak.gainDB * gaussianValue;
        }

        return sumDB + responseCurveShiftDB;
    }

    // State variables
    std::array<float, (size_t)FFT_SIZE / 2 + 1> gainReductionArray{};
    std::array<float, (size_t)FFT_SIZE / 2 + 1> envelopeFollowers{};

    // Compression parameters
    bool clipperActive = false;
    float ratio = 4.0f;
    float kneeWidthDB = 3.0f;
    float attackTimeMs = 10.0f;
    float releaseTimeMs = 100.0f;

    // Smoothing coefficients
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // FFT scaling
    float nyquist;
    float windowComp;
    float scale;
    float dcNyquistScale;

    // Reference to response curve
    GaussianResponseCurve &responseCurve;
    float responseCurveShiftDB = Parameters::responseCurveShiftDB;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralCompressor)
};
