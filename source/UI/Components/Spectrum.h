#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include "SpectralProcessors.h"

template <int FFTSize> class SpectrumDisplay : public juce::Component, private juce::Timer {
    public:
      SpectrumDisplay(SpectralProcessor<FFTSize> &spectralProcessor, double sampleRateInHz)
          : processor(spectralProcessor), sampleRate(sampleRateInHz) {
            smoothedMagnitudes.resize(processor.getMagnitudes().size(), -100.0f);
            startTimerHz(60);
      }

      void paint(juce::Graphics &g) override {
            g.fillAll(juce::Colours::black);

            const auto &mags = processor.getMagnitudes();
            if(mags.empty())
                  return;

            updateSmoothedMagnitudes(mags);

            auto bounds = getLocalBounds();
            juce::Path spectrumPath;
            std::vector<juce::Point<float>> points;
            points.reserve(smoothedMagnitudes.size());

            float nyquistFreq = static_cast<float>(sampleRate) / 2.0f;
            float minFreq = 20.0f;
            float maxFreq = nyquistFreq;

            for(size_t i = 1; i < smoothedMagnitudes.size(); ++i) {
                  float frequency = (i * nyquistFreq) / (smoothedMagnitudes.size() - 1);

                  if(frequency < minFreq)
                        continue;

                  float logFreq = std::log10(frequency);
                  float logMin = std::log10(minFreq);
                  float logMax = std::log10(maxFreq);

                  float x = bounds.getX()
                            + juce::jmap(logFreq, logMin, logMax, 0.0f,
                                         static_cast<float>(bounds.getWidth()));

                  float magnitudeDB = smoothedMagnitudes[i];
                  magnitudeDB = juce::jlimit(-80.0f, 10.0f, magnitudeDB);

                  float y
                   = juce::jmap(magnitudeDB, -80.0f, 10.0f, static_cast<float>(bounds.getBottom()),
                                static_cast<float>(bounds.getY()));

                  points.emplace_back(x, y);
            }

            if(points.size() < 2)
                  return;

            smoothPoints(points);
            spectrumPath.startNewSubPath(points[0]);

            for(size_t i = 1; i < points.size(); ++i) {
                  if(i == 1 || i == points.size() - 1) {
                        spectrumPath.lineTo(points[i]);
                  } else {
                        auto midPoint = juce::Point<float>((points[i - 1].x + points[i].x) * 0.5f,
                                                           (points[i - 1].y + points[i].y) * 0.5f);
                        spectrumPath.quadraticTo(points[i - 1], midPoint);
                  }
            }

            spectrumPath.lineTo(points.back().x, bounds.getBottom());
            spectrumPath.lineTo(points[0].x, bounds.getBottom());
            spectrumPath.closeSubPath();

            juce::ColourGradient gradient(juce::Colours::cyan.withAlpha(0.6f), bounds.getX(),
                                          bounds.getY(), juce::Colours::cyan.withAlpha(0.05f),
                                          bounds.getX(), bounds.getBottom(), false);

            g.setGradientFill(gradient);
            g.fillPath(spectrumPath);

            juce::Path strokePath;
            strokePath.startNewSubPath(points[0]);
            for(size_t i = 1; i < points.size(); ++i) {
                  strokePath.lineTo(points[i]);
            }

            g.setColour(juce::Colours::cyan);
      }

    private:
      void timerCallback() override { repaint(); }

      void smoothPoints(std::vector<juce::Point<float>> &points) {
            if(points.size() < 3)
                  return;

            for(size_t i = 1; i < points.size() - 1; ++i) {
                  points[i].y
                   = points[i - 1].y * 0.3f + points[i].y * 0.4f + points[i + 1].y * 0.3f;
            }
      }

      void updateSmoothedMagnitudes(const auto &newMagnitudes) {
            const float smoothingFactor = 0.05f;

            for(size_t i = 0; i < newMagnitudes.size() && i < smoothedMagnitudes.size(); ++i) {
                  float magnitudeDB;
                  if(newMagnitudes[i] > 1e-10f) {
                        magnitudeDB = 20.0f * std::log10(newMagnitudes[i]);
                  } else {
                        magnitudeDB = -100.0f;
                  }

                  smoothedMagnitudes[i] = smoothedMagnitudes[i] * (1.0f - smoothingFactor)
                                          + magnitudeDB * smoothingFactor;
            }
      }

      SpectralProcessor<FFTSize> &processor;
      std::vector<float> smoothedMagnitudes;
      double sampleRate;
};
