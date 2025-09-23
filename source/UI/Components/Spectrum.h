#pragma once
#include <JuceHeader.h>
#include <cstddef>
#include "SpectralProcessors.h"
#include "juce_graphics/juce_graphics.h"

template <int FFTSize> class SpectrumDisplay : public juce::Component, private juce::Timer {
    public:
      SpectrumDisplay(SpectralProcessor<FFTSize> &spectralProcessor, double sampleRateInHz)
          : processor(spectralProcessor), sampleRate(sampleRateInHz) {
            magnitudes.resize(processor.getMagnitudes().size(), -100.0f);
            startTimerHz(60);
      }

      void updateSpectrumDetail(float newAttack) { spectrumAttack = newAttack; }

      void paint(juce::Graphics &g) override {
            g.fillAll(juce::Colours::black);

            const auto &newMagnitudes = processor.getMagnitudes();
            if(newMagnitudes.empty())
                  return;

            updateMagnitudes(newMagnitudes);

            auto bounds = getLocalBounds();
            juce::Path spectrumPath;
            std::vector<juce::Point<float>> points;
            points.reserve(magnitudes.size());
            points.emplace_back(bounds.getX(), bounds.getBottom());
            float nyquistFreq = static_cast<float>(sampleRate) / 2.0f;
            float minFreq = 20.0f;
            float maxFreq = nyquistFreq;
            float logMin = std::log10(minFreq);
            float logMax = std::log10(maxFreq);
            for(size_t i = 1; i < magnitudes.size(); ++i) {
                  float frequency = (i * nyquistFreq) / (magnitudes.size() - 1);
                  float logFreq = std::log10(frequency);
                  float x = bounds.getX()
                            + juce::jmap(logFreq, logMin, logMax, 0.0f,
                                         static_cast<float>(bounds.getWidth()));

                  float magnitudeDB = magnitudes[i];
                  magnitudeDB = juce::jlimit(-80.0f, 10.0f, magnitudeDB);

                  //
                  //
                  //
                  // ########## POSSIBLE IMPLEMENTATION OF TILTING ##########
                  //
                  // float tiltPerOct = 4.5f; // dB per octave
                  // float refFreq = 1000.0f; // reference frequency (usually 1kHz)
                  //
                  // float octaves = std::log2(frequency / refFreq);
                  // magnitudeDB += tiltPerOct * octaves;
                  //
                  //
                  //

                  float y
                   = juce::jmap(magnitudeDB, -80.0f, 10.0f, static_cast<float>(bounds.getBottom()),
                                static_cast<float>(bounds.getY()));

                  points.emplace_back(x, y);
            }

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

            const float smoothingCoeff = spectrumAttack;

            // Lambda for the smoothing operation
            auto applySmoothing = [&](float &state, float input) {
                  const float coeff = (input > state) ? smoothingCoeff : 1.0f;
                  state = coeff * input + (1.0f - coeff) * state;
            };

            // Forward pass
            float state = points[0].y;
            for(size_t i = 1; i < points.size(); ++i) {
                  applySmoothing(state, points[i].y);
                  points[i].y = state;
            }

            // Backward pass
            state = points.back().y;
            for(auto i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
                  applySmoothing(state, points[i].y);
                  points[i].y = state;
            }
      }

      void updateMagnitudes(const auto &newMagnitudes) {
            const float smoothingFactor = 0.025f;

            for(size_t i = 0; i < newMagnitudes.size() && i < magnitudes.size(); ++i) {
                  float magnitudeDB
                   = newMagnitudes[i] > 1e-10f ? 20.0f * std::log10(newMagnitudes[i]) : -100.0f;
                  magnitudes[i]
                   = magnitudes[i] * (1.0f - smoothingFactor) + magnitudeDB * smoothingFactor;
            }
      }

      SpectralProcessor<FFTSize> &processor;
      std::vector<float> magnitudes;
      double sampleRate;
      float spectrumAttack = 0.1f;
      float spectrumRelease = 0.8f;
};
