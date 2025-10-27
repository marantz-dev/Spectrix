#pragma once

#include "GaussianResponseCurve.h"
#include "PluginParameters.h"
#include "UIutils.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
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

    void setSampleRate(double newSampleRate) { this->sampleRate = newSampleRate; }
    // ##################
    // #                #
    // #  MOUSE EVENTS  #
    // #                #
    // ##################

    // ---------------- MOUSE EVENTS ----------------
    // ---------------- MOUSE EVENTS ----------------
    void mouseDown(const juce::MouseEvent &event) override {
        auto pos = event.position;
        draggedPeakIndex = -1;
        bool clickedOnPeak = false;

        // Check if clicked on an existing peak
        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &peak = gaussians[i];
            float peakX = frequencyToX(peak.frequency);
            float peakY = DBtoY(peak.gainDB + responseCurveShiftDB);
            if(pos.getDistanceFrom({peakX, peakY}) <= 10.0f) {
                clickedOnPeak = true;
                draggedPeakIndex = (int)i;
                dragOffset = {pos.x - peakX, pos.y - peakY};
                initialSigma = peak.sigmaNorm;
                break;
            }
        }

        auto now = juce::Time::getCurrentTime();
        bool isDoubleClick = (now.toMilliseconds() - lastClickTime.toMilliseconds() < 400
                              && lastClickPos.getDistanceFrom(event.position) < 5.0f);

        if(isDoubleClick) {
            if(clickedOnPeak) {
                // Double-click on a peak → delete it
                responseCurve.deletePeak((size_t)draggedPeakIndex);
                draggedPeakIndex = -1;
            } else {
                // Double-click on empty space → add new peak and select it
                auto bounds = getLocalBounds().toFloat();
                float logFreq = xToLogFrequency(pos.x);
                float frequency = std::pow(10.0, logFreq);
                float gainDB = inverseDBWarp(pos.y, bounds) - responseCurveShiftDB;
                responseCurve.addPeak({frequency, gainDB, 0.15f});

                // Select the new peak
                draggedPeakIndex = (int)(gaussians.size() - 1);
                dragOffset = {0.0f, 0.0f}; // center of peak
                initialSigma = gaussians[draggedPeakIndex].sigmaNorm;
                setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            }
        }

        lastClickTime = now;
        lastClickPos = event.position;
        mouseDownPos = event.position;
        repaint();
    }
    void mouseDrag(const juce::MouseEvent &event) override {
        if(draggedPeakIndex < 0 || draggedPeakIndex >= (int)gaussians.size())
            return;

        setMouseCursor(juce::MouseCursor::DraggingHandCursor);

        auto bounds = getLocalBounds().toFloat();
        auto &peak = gaussians[draggedPeakIndex];

        // --------- SHIFT + DRAG ---------
        if(event.mods.isShiftDown()) {
            float deltaY = event.position.y - mouseDownPos.y;

            // Adjust sigma relative to vertical movement
            float sigmaChange = -deltaY / bounds.getHeight(); // moving up increases sigma
            peak.sigmaNorm = juce::jlimit(0.001, 2.0, (double)initialSigma + sigmaChange);

            // X = frequency follows mouse
            float newX = event.position.x;
            newX = juce::jlimit(bounds.getX(), bounds.getRight(), newX);
            float logFreq = xToLogFrequency(newX);
            peak.frequency = std::pow(10.0, juce::jlimit(logMin, logMax, (double)logFreq));
        } else {
            // Normal drag = move both frequency and gain
            float newX = event.position.x - dragOffset.x;
            newX = juce::jlimit(bounds.getX(), bounds.getRight(), newX);
            float logFreq = xToLogFrequency(newX);
            peak.frequency = std::pow(10.0, juce::jlimit(logMin, logMax, (double)logFreq));

            float newY = event.position.y - dragOffset.y;
            newY = juce::jlimit(bounds.getY(), bounds.getBottom(), newY);
            peak.gainDB = inverseDBWarp(newY, bounds) - responseCurveShiftDB;
        }

        repaint();
    }

    void mouseUp(const juce::MouseEvent &) override { draggedPeakIndex = -1; }

    void mouseMove(const juce::MouseEvent &event) override {
        auto pos = event.position;
        hoveredPeakIndex = -1; // reset
        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &peak = gaussians[i];
            float peakX = frequencyToX(peak.frequency);
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
            float peakX = frequencyToX(gaussian.frequency);
            float peakY = DBtoY(gaussian.gainDB + responseCurveShiftDB);

            bool isSelected = ((int)i == draggedPeakIndex) || ((int)i == hoveredPeakIndex);

            if(isSelected) {
                g.setColour(juce::Colours::white); // selected / hovered = white ring
                g.fillEllipse(peakX - 10.0f, peakY - 10.0f, 20.0f, 20.0f);
                g.setColour(juce::Colours::blueviolet);
                g.fillEllipse(peakX - 7.5f, peakY - 7.5f, 15.0f, 15.0f);
            } else {
                g.setColour(juce::Colours::blueviolet);
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

    float frequencyToX(float frequency) const {
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
    double minFreq;
    double maxFreq;
    double logMin;
    double logMax;

    juce::Time lastClickTime;
    juce::Point<float> lastClickPos;

    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;

    const float minDB = Parameters::minDBVisualizer;
    const float maxDB = Parameters::maxDBVisualizer;
    float warpMidpoint = Parameters::warpMidPoint;
    float warpSteepness = Parameters::warpSteepness;
    float responseCurveShiftDB = Parameters::defaultCurveShiftDB;
    bool doubleClickDragMode = false;
    juce::Point<float> mouseDownPos;
};
