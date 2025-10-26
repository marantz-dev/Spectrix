#pragma once
#include "FFTProcessor.h"
#include "GaussianResponseCurve.h"
#include "PluginParameters.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include <JuceHeader.h>
#include <array>
#include <cstddef>
#include <vector>
#include <cmath>

enum CompressorMode { COMPRESSOR, CLIPPER, GATE };

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2>
class SpectralCompressor : public FFTProcessor<FFT_SIZE, NUM_CHANNELS> {
  public:
    SpectralCompressor(GaussianResponseCurve &responseCurveReference)
        : FFTProcessor<FFT_SIZE, NUM_CHANNELS>(), responseCurve(responseCurveReference) {
        envelopeFollowers.fill(0.0f);
    }

    void setCompressorMode(int newMode) { mode = CompressorMode(newMode); }

    std::array<float, FFT_SIZE / 2 + 1> getGainReductionArray() const { return gainReductionArray; }

    void setAttackTime(float timeMs) {
        attackTimeMs = timeMs;
        updateCoefficients();
    }

    void setReleaseTime(float timeMs) {
        releaseTimeMs = timeMs;
        updateCoefficients();
    }

    void setRatio(float newRatio) { ratio = newRatio; }

    void setKnee(float kneeDB) { kneeWidthDB = kneeDB; }

    void prepareToPlay(double newSampleRate) override {
        FFTProcessor<FFT_SIZE, NUM_CHANNELS>::prepareToPlay(newSampleRate);
        this->nyquist = static_cast<float>(newSampleRate) * 0.5f;
        this->windowCompensation = WINDOW_COMPENSATION_FACTOR;
        this->scale = (2.0f / FFT_SIZE) * windowCompensation;
        this->dcNyquistScale = (1.0f / FFT_SIZE) * windowCompensation;
        updateCoefficients();
        envelopeFollowers.fill(0.0f);
    }

  private:
    static constexpr size_t NUM_BINS = FFT_SIZE / 2 + 1;
    static constexpr size_t OVERLAP_FACTOR = 4;
    static constexpr float WINDOW_COMPENSATION_FACTOR = 0.49f;
    static constexpr float MIN_MAGNITUDE_THRESHOLD = 1e-10f;
    static constexpr float MIN_MAGNITUDE_DB = -100.0f;
    static constexpr float MAX_COEFF = 0.99999f;

    void updateCoefficients() {
        if(this->sampleRate <= 0.0)
            return;

        const float hopSize = FFT_SIZE / static_cast<float>(OVERLAP_FACTOR);
        const float timePerHop = hopSize / static_cast<float>(this->sampleRate);

        attackCoeff = std::exp(-timePerHop / (attackTimeMs * 0.001f));
        releaseCoeff = std::exp(-timePerHop / (releaseTimeMs * 0.001f));

        attackCoeff = juce::jlimit(0.0f, MAX_COEFF, attackCoeff);
        releaseCoeff = juce::jlimit(0.0f, MAX_COEFF, releaseCoeff);
    }

    void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {
        const auto gaussianPeaks = responseCurve.getGaussianPeaks();
        if(gaussianPeaks.empty())
            return;

        for(size_t bin = 0; bin < NUM_BINS; ++bin) {
            processSingleBin(transformedBuffer, bin, gaussianPeaks);
        }
    }

    void processSingleBin(std::array<float, FFT_SIZE * 2> &buffer, size_t bin,
                          const std::vector<GaussianPeak> &gaussianPeaks) {
        const float frequency = binToFrequency(bin);
        float magnitude, phase;
        extractMagnitudeAndPhase(buffer, bin, magnitude, phase);
        const float magnitudeDB = magnitudeToDecibels(magnitude);
        const float thresholdDB = calculateGaussianSum(frequency, gaussianPeaks);
        const float gainReductionDB = calculateCompression(magnitudeDB, thresholdDB, bin);
        gainReductionArray[bin] = gainReductionDB;
        const float gainLinear = Decibels::decibelsToGain(-gainReductionDB);
        magnitude *= gainLinear;
        reconstructComplexValue(buffer, bin, magnitude, phase);
    }

    float binToFrequency(size_t bin) const {
        return bin / static_cast<float>(FFT_SIZE) * this->sampleRate;
    }

    bool isDCOrNyquistBin(size_t bin) const { return bin == 0 || bin == FFT_SIZE / 2; }

    void extractMagnitudeAndPhase(const std::array<float, FFT_SIZE * 2> &buffer, size_t bin,
                                  float &magnitude, float &phase) const {
        if(isDCOrNyquistBin(bin)) {
            magnitude = std::abs(buffer[bin]) * dcNyquistScale;
            phase = (buffer[bin] >= 0.0f) ? 0.0f : juce::MathConstants<float>::pi;
        } else {
            const float real = buffer[bin];
            const float imag = buffer[bin + FFT_SIZE];
            magnitude = std::sqrt(real * real + imag * imag) * scale;
            phase = std::atan2(imag, real);
        }
    }

    void reconstructComplexValue(std::array<float, FFT_SIZE * 2> &buffer, size_t bin,
                                 float magnitude, float phase) const {
        if(isDCOrNyquistBin(bin)) {
            magnitude /= dcNyquistScale;
            buffer[bin] = magnitude * std::cos(phase);
        } else {
            magnitude /= scale;
            buffer[bin] = magnitude * std::cos(phase);
            buffer[bin + FFT_SIZE] = magnitude * std::sin(phase);
        }
    }

    float magnitudeToDecibels(float magnitude) const {
        return (magnitude > MIN_MAGNITUDE_THRESHOLD) ? 20.0f * std::log10(magnitude)
                                                     : MIN_MAGNITUDE_DB;
    }

    float decibelsToLinear(float dB) const { return std::pow(10.0f, dB / 20.0f); }

    float calculateCompression(float magnitudeDB, float thresholdDB, size_t bin) {
        switch(mode) {
        case COMPRESSOR: {
            float gainReductionDB = calculateGainReduction(magnitudeDB, thresholdDB);
            return applyEnvelopeFollower(bin, gainReductionDB);
        }
        case CLIPPER: {
            float over = magnitudeDB - thresholdDB;
            float gainReductionDB = (over > 0.0f) ? over : 0.0f;
            return gainReductionDB;
        }
        case GATE: {
            if(magnitudeDB < thresholdDB)
                return applyEnvelopeFollower(bin, 100.0f); // 100 dB = praticamente muto
            else
                return applyEnvelopeFollower(bin, 0.0f); // nessuna riduzione
        }
        default: return 0.0f;
        }
    }

    float calculateGainReduction(float inputDB, float thresholdDB) const {
        const float overThresholdDB = inputDB - thresholdDB;
        const float halfKnee = kneeWidthDB / 2.0f;

        if(overThresholdDB <= -halfKnee) {
            return 0.0f;
        }

        if(overThresholdDB >= halfKnee) {
            return overThresholdDB * (1.0f - 1.0f / ratio);
        }

        const float kneePosition = overThresholdDB + halfKnee;
        const float kneeCurve = (kneePosition * kneePosition) / (2.0f * kneeWidthDB);
        return kneeCurve * (1.0f - 1.0f / ratio);
    }

    float applyEnvelopeFollower(size_t bin, float targetGainReductionDB) {
        const float currentEnvelope = envelopeFollowers[bin];
        const float smoothingCoeff
         = (targetGainReductionDB > currentEnvelope) ? attackCoeff : releaseCoeff;
        const float newEnvelope
         = smoothingCoeff * currentEnvelope + (1.0f - smoothingCoeff) * targetGainReductionDB;
        envelopeFollowers[bin] = newEnvelope;
        return newEnvelope;
    }

    float calculateGaussianSum(float frequency, const std::vector<GaussianPeak> &peaks) const {
        const float responseCurveShiftDB = responseCurve.getResponseCurveShiftDB();
        const float logFreq = std::log10(frequency);
        float thresholdDB = 0.0f;
        for(const auto &peak : peaks) {
            const float peakLogFreq = std::log10(peak.frequency);
            const float logFrequencyDelta = logFreq - peakLogFreq;
            const float exponent
             = -0.5f * (logFrequencyDelta * logFrequencyDelta) / (peak.sigmaNorm * peak.sigmaNorm);
            const float gaussianValue = std::exp(exponent);
            thresholdDB += peak.gainDB * gaussianValue;
        }

        return thresholdDB + responseCurveShiftDB;
    }

    std::array<float, NUM_BINS> gainReductionArray{};
    std::array<float, NUM_BINS> envelopeFollowers{};

    CompressorMode mode = COMPRESSOR;

    float ratio = 4.0f;
    float kneeWidthDB = 3.0f;

    float attackTimeMs = 10.0f;
    float releaseTimeMs = 100.0f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    float nyquist = 0.0f;
    float windowCompensation = 0.0f;
    float scale = 0.0f;
    float dcNyquistScale = 0.0f;

    GaussianResponseCurve &responseCurve;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralCompressor)
};
