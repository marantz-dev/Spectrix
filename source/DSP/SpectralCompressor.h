#include "FFTProcessor.h"
#include "GaussianResponseCurve.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <JuceHeader.h>
#include <vector>

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2>
class SpectralCompressor : public FFTProcessor<FFT_SIZE, NUM_CHANNELS> {
  public:
    SpectralCompressor(GaussianResponseCurve &responseCurveReference)
        : FFTProcessor<FFT_SIZE, NUM_CHANNELS>(), responseCurve(responseCurveReference) {}

  private:
    void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {
        auto gaussianPeaks = responseCurve.getGaussianPeaks();
        if(gaussianPeaks.empty())
            return;

        const float nyquist = static_cast<float>(this->sampleRate) * 0.5f;

        const float zeroDB_Reference = 500;
        // TODO: find out about the zeroDB reference
        for(size_t bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            float frequency
             = (static_cast<float>(bin) / FFT_SIZE) * static_cast<float>(this->sampleRate);

            if(frequency < 20.0f || frequency > nyquist)
                continue;

            float magnitude, phase;
            if(bin == 0 || bin == FFT_SIZE / 2) {
                magnitude = std::abs(transformedBuffer[bin]);
                phase = (transformedBuffer[bin] >= 0.0f) ? 0.0f : juce::MathConstants<float>::pi;
            } else {
                float real = transformedBuffer[bin];
                float imag = transformedBuffer[bin + FFT_SIZE];
                magnitude = std::sqrt(real * real + imag * imag);
                phase = std::atan2(imag, real);
            }

            float thresholdDB = calculateGaussianSum(frequency, gaussianPeaks);

            float magnitudeDB = juce::Decibels::gainToDecibels(magnitude / zeroDB_Reference);

            if(magnitudeDB > thresholdDB) {
                magnitude = juce::Decibels::decibelsToGain(thresholdDB) * zeroDB_Reference;
            }

            if(bin == 0 || bin == FFT_SIZE / 2) {
                transformedBuffer[bin] = magnitude * std::cos(phase);
            } else {
                transformedBuffer[bin] = magnitude * std::cos(phase);
                transformedBuffer[bin + FFT_SIZE] = magnitude * std::sin(phase);
            }
        }
    }

    float calculateGaussianSum(float frequency, const std::vector<GaussianPeak> &peaks) {
        float logFreq = std::log10(frequency);
        float sumDB = 0.0f;

        for(const auto &peak : peaks) {
            float peakLogFreq = std::log10(peak.frequency);
            float dx = logFreq - peakLogFreq;

            float gaussianValue = std::exp(-0.5f * (dx * dx) / (peak.sigmaNorm * peak.sigmaNorm));
            sumDB += peak.gainDB * gaussianValue;
        }

        return sumDB;
    }

    GaussianResponseCurve &responseCurve;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralCompressor)
};
