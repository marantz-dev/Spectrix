#pragma once

#include "GaussianResponseCurve.h"
#include "PluginParameters.h"
#include "UIutils.h"
#include "juce_graphics/juce_graphics.h"
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>
#include <numbers>

class ResponseCurve : public juce::Component {
  public:
    ResponseCurve(GaussianResponseCurve &responseCurveReference, double sampleRateHz)
        : responseCurve(responseCurveReference), sampleRate(sampleRateHz),
          gaussians(responseCurveReference.getGaussianPeaks()) {
        updateLogFreqBounds();
    }

    void paint(juce::Graphics &g) override {
        responseCurveShiftDB = responseCurve.getResponseCurveShiftDB();
        drawGaussianCurves(g);
        drawSumOfGaussians(g);
        drawGaussianPeaks(g);
    }
    // ##################
    // #                #
    // #  MOUSE EVENTS  #
    // #                #
    // ##################

    void mouseDown(const juce::MouseEvent &event) override {
        auto bounds = getLocalBounds().toFloat();
        auto pos = event.position;
        bool clickedOnPeak = false;
        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &peak = gaussians[i];
            float peakX = logFrequencyToX(peak.frequency);
            float peakY = DBtoY(peak.gainDB + responseCurveShiftDB); // Add shift BEFORE converting
            float dist = pos.getDistanceFrom(juce::Point<float>(peakX, peakY));
            if(dist <= 10.0f) {
                clickedOnPeak = true;
                draggedPeakIndex = (int)i;
                dragOffset = {pos.x - peakX, pos.y - peakY};
                initialSigma = peak.sigmaNorm;
                break;
            }
        }
        auto now = juce::Time::getCurrentTime();
        if((now.toMilliseconds() - lastClickTime.toMilliseconds() < 400)
           && lastClickPos.getDistanceFrom(event.position) < 5.0f) {
            if(clickedOnPeak) {
                responseCurve.deletePeak((size_t)draggedPeakIndex);
                draggedPeakIndex = -1;
                if(gaussians.empty()) {
                    responseCurve.addPeak({1000.0f, 0.0f, 0.25f});
                }
            } else {
                float logFreq = xToLogFrequency(event.position.x);
                float frequency = std::pow(10.0f, logFreq);
                float gainDB = inverseDBWarp(event.position.y, bounds) - responseCurveShiftDB;
                responseCurve.addPeak({frequency, gainDB, 0.15f});
            }
        }
        lastClickTime = now;
        lastClickPos = event.position;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override {
        if(draggedPeakIndex >= 0 && draggedPeakIndex < (int)gaussians.size()) {
            auto bounds = getLocalBounds().toFloat();
            auto &peak = gaussians[draggedPeakIndex];
            if(!event.mods.isShiftDown()) {
                float newX = event.position.x - dragOffset.x;
                newX = juce::jlimit(bounds.getX(), bounds.getRight(), newX);
                float logFreq = xToLogFrequency(newX);
                float newFrequency = std::pow(10.0f, logFreq);
                float newY = event.position.y - dragOffset.y;
                newY = juce::jlimit(bounds.getY(), bounds.getBottom(), newY);
                float gainDB = inverseDBWarp(newY, bounds) - responseCurveShiftDB;
                gaussians[draggedPeakIndex].frequency = newFrequency;
                gaussians[draggedPeakIndex].gainDB = gainDB;
            } else {
                float deltaX = event.position.x - event.mouseDownPosition.x;
                float sigmaChange = deltaX / bounds.getWidth() * 0.5f;
                float newSigma = initialSigma + sigmaChange;
                newSigma = juce::jlimit(0.001f, 2.0f, newSigma);
                gaussians[draggedPeakIndex].sigmaNorm = newSigma;
            }
            repaint();
        }
    }

    void mouseMove(const juce::MouseEvent &event) override {
        auto pos = event.position;
        hoveredPeakIndex = -1; // reset
        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &peak = gaussians[i];
            float peakX = logFrequencyToX(peak.frequency);
            float peakY = DBtoY(peak.gainDB + responseCurveShiftDB);
            if(pos.getDistanceFrom({peakX, peakY}) <= 10.0f) { // hover radius
                hoveredPeakIndex = (int)i;
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
                break;
            } else {
                setMouseCursor(juce::MouseCursor::NormalCursor);
            }

            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent &) override { draggedPeakIndex = -1; }

  private:
    // ##################
    // #                #
    // #  DRAW METHODS  #
    // #                #
    // ##################

    void drawGaussianCurves(juce::Graphics &g) {
        auto bounds = getLocalBounds().toFloat();
        for(auto &gaussian : gaussians) {
            juce::Path path;
            for(int x = 0; x <= (int)bounds.getWidth(); ++x) {
                float logFrequency = xToLogFrequency(bounds.getX() + x);
                float gaussianValue = gaussianAtLogFrequency(logFrequency, gaussian);
                float y = DBtoY(gaussianValue
                                + responseCurveShiftDB); // Add shift to value before converting
                x == 0 ? path.startNewSubPath(bounds.getX(), y) : path.lineTo(bounds.getX() + x, y);
            }
            g.setColour(juce::Colours::aliceblue.withAlpha(0.2f));
            g.strokePath(path, juce::PathStrokeType(2.0f));
        }
    }

    void drawSumOfGaussians(juce::Graphics &g) {
        auto bounds = getLocalBounds().toFloat();
        juce::Path path;
        for(int x = 0; x <= bounds.getWidth(); ++x) {
            float sumDB = 0.0f;
            float logFreq = xToLogFrequency(bounds.getX() + x);
            for(const auto &gauss : gaussians) {
                float gaussianValue = gaussianAtLogFrequency(logFreq, gauss);
                sumDB += gaussianValue;
            }
            float y = DBtoY(sumDB + responseCurveShiftDB); // Add shift to value before converting
            x == 0 ? path.startNewSubPath(bounds.getX(), y) : path.lineTo(bounds.getX() + x, y);
        }
        g.setColour(juce::Colours::yellow.withAlpha(0.8f).brighter());
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

    void drawGaussianPeaks(juce::Graphics &g) {
        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &gaussian = gaussians[i];
            float peakX = logFrequencyToX(gaussian.frequency);
            float peakY = DBtoY(gaussian.gainDB + responseCurveShiftDB);

            if((int)i == hoveredPeakIndex) {
                g.setColour(juce::Colours::white); // hovered = white
                g.fillEllipse(peakX - 10.0f, peakY - 10.0f, 20.0f, 20.0f);
                g.setColour(juce::Colours::magenta); // normal = yellow
                g.fillEllipse(peakX - 7.5f, peakY - 7.5f, 15.0f, 15.0f);
            } else {
                g.setColour(juce::Colours::magenta); // normal = yellow
                g.fillEllipse(peakX - 7.5f, peakY - 7.5f, 15.0f, 15.0f);
            }
        }
    }

    // #############
    // #           #
    // #  HELPERS  #
    // #           #
    // #############

    float DBtoY(float magnitudeDB) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        float warpedDB = DBWarp(magnitudeDB);
        return juce::jmap(warpedDB, minDB, maxDB, bounds.getBottom(), bounds.getY());
    }

    float YtoDB(float yPos) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        float warpedDB = juce::jmap(yPos, bounds.getBottom(), bounds.getY(), minDB, maxDB);
        return inverseDBWarp(yPos, bounds);
    }

    float xToLogFrequency(float x) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        return juce::jmap<float>(x, bounds.getX(), bounds.getRight(), (float)logMin, (float)logMax);
    }

    float logFrequencyToX(float frequency) const {
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        float logFreq = std::log10(frequency);
        return juce::jmap<float>(logFreq, (float)logMin, (float)logMax, bounds.getX(),
                                 bounds.getRight());
    }

    float gaussianAtLogFrequency(float logFrequency, const GaussianPeak &gaussian) const {
        float peakLogFrequency = std::log10(gaussian.frequency);
        float dx = logFrequency - peakLogFrequency;
        float exponent = -0.5f * (dx * dx) / (gaussian.sigmaNorm * gaussian.sigmaNorm);
        float gaussianLinear = std::exp(exponent);
        return gaussian.gainDB * gaussianLinear;
    }

    void updateLogFreqBounds() {
        minFreq = 20.0;
        maxFreq = sampleRate * 0.5;
        logMin = std::log10(minFreq);
        logMax = std::log10(maxFreq);
    }

    GaussianResponseCurve &responseCurve;
    std::vector<GaussianPeak> &gaussians;

    int draggedPeakIndex = -1;
    int hoveredPeakIndex = -1; // -1 = no peak hovered
    juce::Point<float> dragOffset;

    double sampleRate = 44100.0;
    double minFreq = 20.0;
    double maxFreq = 22050.0;
    double logMin = std::log10(20.0);
    double logMax = std::log10(22050.0);

    juce::Time lastClickTime;
    juce::Point<float> lastClickPos;

    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;

    const float minDB = Parameters::minDBVisualizer;
    const float maxDB = Parameters::maxDBVisualizer;
    float warpMidpoint = Parameters::warpMidPoint;
    float warpSteepness = Parameters::warpSteepness;
    float responseCurveShiftDB = Parameters::defaultCurveShiftDB;
};
