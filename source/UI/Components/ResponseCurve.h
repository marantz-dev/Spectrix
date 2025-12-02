#pragma once

#include "GaussianResponseCurve.h"
#include "PluginParameters.h"
#include "UIutils.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class ResponseCurve : public juce::Component {
  public:
    ResponseCurve(GaussianResponseCurve &responseCurveReference, double sampleRateHz)
        : responseCurve(responseCurveReference), sampleRate(sampleRateHz),
          gaussians(responseCurveReference.getGaussianPeaks()) {
        dragInfoLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        dragInfoLabel.setColour(juce::Label::backgroundColourId,
                                juce::Colours::black.withAlpha(0.7f));
        dragInfoLabel.setJustificationType(juce::Justification::centred);
        dragInfoLabel.setVisible(false);
        dragInfoLabel.setInterceptsMouseClicks(false, false); // don't block dragging
        addAndMakeVisible(dragInfoLabel);
        updateLogFreqBounds();
    }

    void paint(juce::Graphics &g) override {
        responseCurveShiftDB = responseCurve.getResponseCurveShiftDB();
        if(responseCurve.getGaussianPeaks().size() == 0)
            responseCurve.addPeak({1000.0f, 0.0f, 0.25f});
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

    void mouseDown(const juce::MouseEvent &event) override {
        auto pos = event.position;
        draggedPeakIndex = -1;
        bool clickedOnPeak = false;
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
                responseCurve.deletePeak((size_t)draggedPeakIndex);
                if(responseCurve.getGaussianPeaks().size() == 0)
                    responseCurve.addPeak({1000.0f, 0.0f, 0.25f});
                draggedPeakIndex = -1;
            } else {
                auto bounds = getLocalBounds().toFloat();
                float logFreq = xToLogFrequency(pos.x);
                float frequency = std::pow(10.0, logFreq);
                float gainDB = inverseDBWarp(pos.y, bounds) - responseCurveShiftDB;
                responseCurve.addPeak({frequency, gainDB, 0.15f});

                draggedPeakIndex = (int)(gaussians.size() - 1);
                dragOffset = {0.0f, 0.0f};
                initialSigma = gaussians[draggedPeakIndex].sigmaNorm;
                setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            }
        }
        lastClickTime = now;
        lastClickPos = event.position;
        mouseDownPos = event.position;
        // repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override {
        if(draggedPeakIndex < 0 || draggedPeakIndex >= (int)gaussians.size())
            return;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        auto bounds = getLocalBounds().toFloat();
        auto &peak = gaussians[draggedPeakIndex];
        if(event.mods.isShiftDown()) {
            float deltaY = event.position.y - mouseDownPos.y;
            float sigmaChange = -deltaY / bounds.getHeight();
            peak.sigmaNorm = juce::jlimit(0.025, 2.0, (double)initialSigma + sigmaChange);
            float logFreq
             = xToLogFrequency(juce::jlimit(bounds.getX(), bounds.getRight(), event.position.x));
            peak.frequency = std::pow(10.0, juce::jlimit(logMin, logMax, (double)logFreq));
        } else {
            float newX
             = juce::jlimit(bounds.getX(), bounds.getRight(), event.position.x - dragOffset.x);
            float newY
             = juce::jlimit(bounds.getY(), bounds.getBottom(), event.position.y - dragOffset.y);
            peak.frequency = std::pow(10.0, xToLogFrequency(newX));
            peak.gainDB = inverseDBWarp(newY, bounds) - responseCurveShiftDB;
        }

        dragInfoLabel.setVisible(true);
        dragInfoLabel.setText(juce::String(peak.frequency, 1) + " Hz", juce::dontSendNotification);

        float peakX = frequencyToX(peak.frequency);
        float peakY = DBtoY(peak.gainDB + responseCurveShiftDB);
        int labelWidth = 80;
        int labelHeight = 18;

        float x = peakX - labelWidth / 2.0f;
        float y = peakY - 40;

        x = juce::jlimit(bounds.getX(), bounds.getRight() - labelWidth, x);
        y = juce::jlimit(bounds.getY(), bounds.getBottom() - labelHeight, y);

        dragInfoLabel.setBounds((int)x, (int)y, labelWidth, labelHeight);

        // repaint();
    }

    void mouseUp(const juce::MouseEvent &) override {
        draggedPeakIndex = -1;
        dragInfoLabel.setVisible(false);
    }

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

            // repaint();
        }
    }

  private:
    // ###################
    // #                 #
    // #  PAINT METHODS  #
    // #                 #
    // ###################

    void drawGaussianCurves(juce::Graphics &g) {
        auto bounds = getLocalBounds().toFloat();
        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &gaussian = gaussians[i];

            juce::Colour curveColour = getPeakColour(gaussian.frequency);

            bool isSelected = ((int)i == draggedPeakIndex) || ((int)i == hoveredPeakIndex);

            float alpha = isSelected ? 0.4f : 0.205f;
            float strokeWidth = isSelected ? 2.5f : 2.0f;

            juce::Path path;
            for(int x = 0; x <= (int)bounds.getWidth(); ++x) {
                float logFrequency = xToLogFrequency(bounds.getX() + x);
                float gaussianValue = gaussianAtLogFrequency(logFrequency, gaussian);
                float y = DBtoY(gaussianValue + responseCurveShiftDB);
                if(x == 0 || x == (int)bounds.getWidth())
                    y = DBtoY(responseCurveShiftDB);
                x == 0 ? path.startNewSubPath(bounds.getX(), y) : path.lineTo(bounds.getX() + x, y);
            }

            path.closeSubPath();
            g.setColour(curveColour.withAlpha(alpha - 0.2f));
            g.fillPath(path);

            g.setColour(curveColour.withAlpha(alpha));
            g.strokePath(path, juce::PathStrokeType(strokeWidth));
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
            juce::Colour peakColour = getPeakColour(gaussian.frequency);

            if(isSelected) {
                g.setColour(juce::Colours::white); // selected / hovered = white ring
                g.fillEllipse(peakX - 10.0f, peakY - 10.0f, 20.0f, 20.0f);
                g.setColour(peakColour);
                g.fillEllipse(peakX - 7.5f, peakY - 7.5f, 15.0f, 15.0f);
            } else {
                g.setColour(juce::Colours::grey); // selected / hovered = white ring
                g.fillEllipse(peakX - 9.0f, peakY - 9.0f, 18.0f, 18.0f);
                g.setColour(peakColour);
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

    juce::Colour getPeakColour(double frequency) const {
        float freq = juce::jlimit((float)minFreq, (float)maxFreq, (float)frequency);

        if(freq <= 250.0f) {
            float localT = juce::jmap<float>(freq, (float)minFreq, 250.0f, 0.0f, 1.0f);
            return juce::Colours::red.interpolatedWith(juce::Colours::magenta, localT);
        } else if(freq <= 1000.0f) {
            float localT = juce::jmap<float>(freq, 250.0f, 1000.0f, 0.0f, 1.0f);
            return juce::Colours::magenta.interpolatedWith(juce::Colours::purple, localT);
        } else if(freq <= 4000.0f) {
            float localT = juce::jmap<float>(freq, 1000.0f, 4000.0f, 0.0f, 1.0f);
            return juce::Colours::purple.interpolatedWith(juce::Colours::blue, localT);
        } else {
            float localT = juce::jmap<float>(freq, 4000.0f, (float)maxFreq, 0.0f, 1.0f);
            localT = juce::jlimit(0.0f, 1.0f, localT);
            return juce::Colours::blue.interpolatedWith(juce::Colours::cyan, localT);
        }
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

    // ########## Dragging ##########

    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;
    bool doubleClickDragMode = false;
    juce::Point<float> mouseDownPos;
    juce::Label dragInfoLabel;

    const float minDB = Parameters::minDBVisualizer;
    const float maxDB = Parameters::maxDBVisualizer;
    float warpMidpoint = Parameters::warpMidPoint;
    float warpSteepness = Parameters::warpSteepness;
    float responseCurveShiftDB = Parameters::defaultCurveShiftDB;
};
