#pragma once

#include "GaussianResponseCurve.h"
#include "PluginParameters.h"
#include "juce_graphics/juce_graphics.h"
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <numbers>

class ResponseCurve : public juce::Component {
  public:
    ResponseCurve(GaussianResponseCurve &responseCurveReference, double sampleRateHz)
        : responseCurve(responseCurveReference), sampleRate(sampleRateHz) {
        updateLogFreqBounds();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();

        updateGuiPeaks(bounds);
        DBG(guiGaussians.size());

        drawGaussianCurves(g, bounds);
        drawSumOfGaussians(g);
        drawGaussianPeaks(g);
        // drawZeroDbLine(g, bounds);
    }
    // ##################
    // #                #
    // #  MOUSE EVENTS  #
    // #                #
    // ##################

    void mouseDown(const juce::MouseEvent &event) override {
        auto pos = event.position;
        auto bounds = getLocalBounds().toFloat();
        bool clickedOnPeak = false;

        // find nearest peak within 10px
        for(size_t i = 0; i < guiGaussians.size(); ++i) {
            auto &peak = guiGaussians[i];
            float dist = pos.getDistanceFrom(juce::Point<float>(peak.peakX, peak.peakY));
            if(dist <= 10.0f) {
                clickedOnPeak = true;
                draggedPeakIndex = (int)i;
                dragOffset = {pos.x - peak.peakX, pos.y - peak.peakY};
                break;
            }
        }

        auto now = juce::Time::getCurrentTime();
        if((now.toMilliseconds() - lastClickTime.toMilliseconds() < 400)
           && lastClickPos.getDistanceFrom(event.position) < 5.0f) {
            if(clickedOnPeak) {
                responseCurve.deletePeak((size_t)draggedPeakIndex);
                draggedPeakIndex = -1;
            } else {
                float logFreq = juce::jmap<float>(event.position.x, bounds.getX(),
                                                  bounds.getRight(), (float)logMin, (float)logMax);
                float frequency = std::pow(10.0f, logFreq);

                // Inverse warp the Y position to get actual dB value
                float gainDB = inverseDBWarp(event.position.y, bounds);

                responseCurve.addPeak({frequency, gainDB, 0.05f});
            }
        }
        lastClickTime = now;
        lastClickPos = event.position;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override {
        if(draggedPeakIndex >= 0) {
            auto bounds = getLocalBounds().toFloat();
            auto &peakData = responseCurve.getGaussianPeaks()[draggedPeakIndex];
            auto &guiPeak = guiGaussians[draggedPeakIndex];

            if(!event.mods.isShiftDown()) {
                float newX = event.position.x - dragOffset.x;
                newX = juce::jlimit(bounds.getX(), bounds.getRight(), newX);
                guiPeak.peakX = newX;

                float logFreq = juce::jmap<float>(newX, bounds.getX(), bounds.getRight(),
                                                  (float)logMin, (float)logMax);
                peakData.frequency = std::pow(10.0f, logFreq);

                float newY = event.position.y - dragOffset.y;
                newY = juce::jlimit(bounds.getY(), bounds.getBottom(), newY);
                guiPeak.peakY = newY;

                // Inverse warp the Y position to get actual dB value
                float gainDB = inverseDBWarp(newY, bounds);
                peakData.gainDB = gainDB;

                repaint();
            } else {
                float deltaX = event.position.x - initialMouseX;
                float sigmaChange = deltaX / bounds.getWidth();
                float newSigma = peakData.sigmaNorm + sigmaChange;
                peakData.sigmaNorm = newSigma;
                guiPeak.sigma = newSigma * bounds.getWidth();
                repaint();
                initialMouseX = event.position.x;
                initialSigma = peakData.sigmaNorm;
            }
        }
    }

    void mouseUp(const juce::MouseEvent &) override { draggedPeakIndex = -1; }

  private:
    struct GUIGaussian {
        float peakX;
        float peakY;
        float sigma;
        float peakDB;
        float frequency;
    };

    float DBtoY(float magnitudeDB, juce::Rectangle<float> bounds) const {
        float warpedDB = DBWarp(magnitudeDB);
        return juce::jmap(warpedDB, minDB, maxDB, bounds.getBottom(), bounds.getY());
    }
    float YtoDB(float yPos, juce::Rectangle<float> bounds) const {
        float warpedDB = juce::jmap(yPos, bounds.getBottom(), bounds.getY(), minDB, maxDB);
        return inverseDBWarp(yPos, bounds);
    }

    float xToFrequency(float x) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        float logFreq
         = juce::jmap<float>(x, bounds.getX(), bounds.getRight(), (float)logMin, (float)logMax);
        return std::pow(10.0f, logFreq);
    }

    float frequencyToX(float frequency) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        float logFreq = std::log10(frequency);
        return juce::jmap<float>(logFreq, (float)logMin, (float)logMax, bounds.getX(),
                                 bounds.getRight());
    }

    float DBWarp(float magnitudeDB) const {
        float norm = juce::jmap(magnitudeDB, minDB, maxDB, 0.0f, 1.0f);
        float x = (norm - 0.44f) * 5.0f;
        float sigmoid = 1.0f / (1.0f + std::exp(-x));
        return juce::jmap(sigmoid, 0.0f, 1.0f, minDB, maxDB);
    }

    float inverseDBWarp(float yPos, juce::Rectangle<float> bounds) const {
        float warpedDB = juce::jmap(yPos, bounds.getBottom(), bounds.getY(), minDB, maxDB);
        float sigmoid = juce::jmap(warpedDB, minDB, maxDB, 0.0f, 1.0f);
        sigmoid = juce::jlimit(0.001f, 0.999f, sigmoid);
        float x = -std::log(1.0f / sigmoid - 1.0f);
        float norm = (x / 5.0f) + 0.44f;
        norm = juce::jlimit(0.0f, 1.0f, norm);
        return juce::jmap(norm, 0.0f, 1.0f, minDB, maxDB);
    }

    void updateGuiPeaks(juce::Rectangle<float> bounds) {
        guiGaussians.clear();

        auto peaks = responseCurve.getGaussianPeaks();
        for(auto &p : peaks) {
            float logFreq = std::log10(p.frequency);
            float px = juce::jmap<float>((float)logFreq, (float)logMin, (float)logMax,
                                         bounds.getX(), bounds.getRight());
            float warpedDB = DBWarp(p.gainDB);
            float py = juce::jmap<float>(warpedDB, minDB, maxDB, bounds.getBottom(), bounds.getY());
            guiGaussians.push_back({px, py, p.sigmaNorm, p.gainDB, p.frequency});
        }
    }

    // ###################
    // #                 #
    // #  PAINT METHODS  #
    // #                 #
    // ###################

    void drawGaussianCurves(juce::Graphics &g, juce::Rectangle<float> bounds) {
        for(auto &gaussian : guiGaussians) {
            juce::Path path;
            bool firstPoint = true;

            // Draw in log-frequency space to match DSP
            for(int x = 0; x <= (int)bounds.getWidth(); x += 2) {
                // Convert pixel X to log frequency
                float logFreq = juce::jmap<float>((float)x, 0.0f, bounds.getWidth(), (float)logMin,
                                                  (float)logMax);
                float peakLogFreq = std::log10(gaussian.frequency);

                // Calculate Gaussian in log-frequency space
                float dx = logFreq - peakLogFreq;
                float exponent = -0.5f * (dx * dx) / (gaussian.sigma * gaussian.sigma);
                float gaussianValue = std::exp(exponent);
                float valueDB = gaussian.peakDB * gaussianValue;

                // Apply warping to the calculated dB value
                float warpedDB = DBWarp(valueDB);
                float y = juce::jmap(warpedDB, minDB, maxDB, bounds.getBottom(), bounds.getY());

                if(firstPoint) {
                    path.startNewSubPath(bounds.getX() + x, y);
                    firstPoint = false;
                } else {
                    path.lineTo(bounds.getX() + x, y);
                }
            }

            g.setColour(juce::Colours::aliceblue.withAlpha(0.2f));
            g.strokePath(path, juce::PathStrokeType(2.0f));
        }
    }

    void drawSumOfGaussians(juce::Graphics &g) {
        auto bounds = getLocalBounds().toFloat();
        if(guiGaussians.empty())
            return;

        juce::Path path;

        for(int x = 0; x <= bounds.getWidth(); ++x) {
            // Convert pixel X to frequency
            float logFreq
             = juce::jmap<float>((float)x, 0.0f, bounds.getWidth(), (float)logMin, (float)logMax);

            float sumDB = 0.0f;

            // Sum each Gaussian in log-frequency space (matches DSP)
            for(const auto &gauss : guiGaussians) {
                float peakLogFreq = std::log10(gauss.frequency);
                float dx = logFreq - peakLogFreq;
                float exponent = -0.5f * (dx * dx) / (gauss.sigma * gauss.sigma);
                float gaussianValue = std::exp(exponent);
                sumDB += gauss.peakDB * gaussianValue;
            }

            sumDB += responseCurveShiftDB;
            float warpedDB = DBWarp(sumDB);
            float y = juce::jmap(warpedDB, minDB, maxDB, bounds.getBottom(), bounds.getY());

            if(x == 0)
                path.startNewSubPath(bounds.getX() + x, y);
            else
                path.lineTo(bounds.getX() + x, y);
        }

        g.setColour(juce::Colours::orange.withAlpha(0.8f));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

    void drawGaussianPeaks(juce::Graphics &g) {
        for(auto &peak : guiGaussians) {
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(peak.peakX - 4.0f, peak.peakY - 4.0f, 8.0f, 8.0f);
        }
    }

    void updateLogFreqBounds() {
        minFreq = 20.0;
        maxFreq = sampleRate * 0.5;
        logMin = std::log10(minFreq);
        logMax = std::log10(maxFreq);
    }

    GaussianResponseCurve &responseCurve;
    std::vector<GUIGaussian> guiGaussians;

    int draggedPeakIndex = -1;
    juce::Point<float> dragOffset;

    double sampleRate = 44100.0;
    const float minDB = Parameters::minDBVisualizer;
    const float maxDB = Parameters::maxDBVisualizer;
    double minFreq = 20.0;
    double maxFreq = 22050.0;
    double logMin = std::log10(20.0);
    double logMax = std::log10(22050.0);

    juce::Time lastClickTime;
    juce::Point<float> lastClickPos;

    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;

    float warpMidpoint = Parameters::warpMidPoint;
    float warpSteepness = Parameters::warpSteepness;
    float responseCurveShiftDB = Parameters::responseCurveShiftDB;
};
