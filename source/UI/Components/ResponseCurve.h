#pragma once

#include "GaussianResponseCurve.h"
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

        updateGuiPeaks(bounds); // recompute every frame

        drawGaussianCurves(g, bounds);
        drawSumOfGaussians(g);
        drawGaussianPeaks(g);
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
            float dist = pos.getDistanceFrom(Point<float>(peak.peakX, peak.peakY));
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
                auto &peakData = responseCurve.getGaussianPeaks();
                float middleY = bounds.getCentreY();
                float gainDB;
                if(event.position.y <= middleY)
                    gainDB
                     = juce::jmap<float>(event.position.y, middleY, bounds.getY(), 0.0f, maxDB);
                else
                    gainDB = juce::jmap<float>(event.position.y, middleY, bounds.getBottom(), 0.0f,
                                               minDB);
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
            float middleY = bounds.getCentreY();
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

                float gainDB;
                if(newY <= middleY)
                    gainDB = juce::jmap<float>(newY, middleY, bounds.getY(), 0.0f, maxDB);
                else
                    gainDB = juce::jmap<float>(newY, middleY, bounds.getBottom(), 0.0f, minDB);

                peakData.gainDB = gainDB;

                repaint();
            } else {
                float deltaX = event.position.x - initialMouseX;
                float sigmaChange = deltaX / bounds.getWidth();
                float newSigma = juce::jlimit(0.01f, 0.5f, initialSigma + sigmaChange);
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
    };

    void updateGuiPeaks(juce::Rectangle<float> bounds) {
        guiGaussians.clear();
        float middleY = bounds.getCentreY();

        auto peaks = responseCurve.getGaussianPeaks();
        for(auto &p : peaks) {
            float logFreq = std::log10(p.frequency);
            float px = juce::jmap<float>((float)logFreq, (float)logMin, (float)logMax,
                                         bounds.getX(), bounds.getRight());

            float py;
            if(p.gainDB >= 0)
                py = juce::jmap<float>(p.gainDB, 0.0f, maxDB, middleY, bounds.getY());
            else
                py = juce::jmap<float>(p.gainDB, minDB, 0.0f, getBottom(), middleY);

            guiGaussians.push_back({px, py, p.sigmaNorm * bounds.getWidth(), p.gainDB});
        }
    }

    // ###################
    // #                 #
    // #  PAINT METHODS  #
    // #                 #
    // ###################

    void drawGaussianCurves(juce::Graphics &g, juce::Rectangle<float> bounds) {
        float middleY = bounds.getCentreY();

        for(auto &gaussian : guiGaussians) {
            juce::Path path;
            path.startNewSubPath(bounds.getX(), middleY);

            float denom = 1.0f / (gaussian.sigma * std::sqrt(2.0f * std::numbers::pi));

            for(int x = 0; x <= (int)bounds.getWidth(); x += 2) {
                float dx = x - gaussian.peakX;
                float gaussVal
                 = denom * std::exp(-(dx * dx) / (2.0f * gaussian.sigma * gaussian.sigma));
                float y = middleY - gaussVal / denom * (middleY - gaussian.peakY);
                path.lineTo((float)x, y);
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
        float middleY = bounds.getCentreY();

        // Step across the screen
        for(int x = 0; x <= bounds.getWidth(); ++x) {
            float sumDB = 0.0f;

            // Sum each gaussian in dB space
            for(const auto &gauss : guiGaussians) {
                float dx = x - gauss.peakX;
                float exponent = -0.5f * (dx * dx) / (gauss.sigma * gauss.sigma);
                float valueDB = gauss.peakDB * std::exp(exponent); // gain curve
                sumDB += valueDB;
            }

            // Convert sumDB to Y pixel position:
            float y;
            if(sumDB >= 0)
                y = juce::jmap(sumDB, 0.0f, maxDB, middleY, bounds.getY());
            else
                y = juce::jmap(sumDB, minDB, 0.0f, bounds.getBottom(), middleY);

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

  private:
    GaussianResponseCurve &responseCurve;
    std::vector<GUIGaussian> guiGaussians;

    int draggedPeakIndex = -1;
    juce::Point<float> dragOffset;

    double sampleRate = 44100.0;
    const float minDB = -48.0f;
    const float maxDB = 48.0f;
    double minFreq = 20.0;
    double maxFreq = 22050.0;
    double logMin = std::log10(20.0);
    double logMax = std::log10(22050.0);

    juce::Time lastClickTime;
    juce::Point<float> lastClickPos;

    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;
};
