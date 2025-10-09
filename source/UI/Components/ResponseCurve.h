
#include "PluginParameters.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>
#include <array>
#include <cstddef>
class ResponseCurve : public juce::Component {
  public:
    ResponseCurve() { addGaussian(0.5f, 0.0f, 0.15f); }

    void addGaussian(float px, float py, float sigma) {
        gaussians.push_back({px, py, sigma, colours[colourIndex]});
        colourIndex = (colourIndex + 1) % (int)colours.size();
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();

        auto midY = bounds.getCentreY();

        for(auto &gauss : gaussians) {
            juce::Path path;

            float mean = gauss.peakX * bounds.getWidth();
            float sigma = gauss.sigmaNorm * bounds.getWidth();

            path.startNewSubPath(0, midY);
            for(int x = 0; x < bounds.getWidth(); ++x) {
                float dx = x - mean;
                float y = std::exp(-0.5f * (dx * dx) / (sigma * sigma));

                float yPos
                 = juce::jmap(y, 0.0f, 1.0f, midY, midY + gauss.peakY * bounds.getHeight() * 0.5f);

                path.lineTo((float)x, yPos);
            }
            path.lineTo(bounds.getRight(), midY);
            path.closeSubPath();

            g.setColour(gauss.colour.withAlpha(0.1f));
            g.fillPath(path);
        }

        sumPoints.clear();
        juce::Path sumPath;
        sumPath.startNewSubPath(0, midY);

        for(int x = 0; x < bounds.getWidth(); ++x) {
            float sumY = 0.0f;

            for(auto &gauss : gaussians) {
                float mean = gauss.peakX * bounds.getWidth();
                float sigma = gauss.sigmaNorm * bounds.getWidth();

                float dx = x - mean;
                float y = std::exp(-0.5f * (dx * dx) / (sigma * sigma));

                float peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;
                float offset = peakPixelY - midY;

                sumY += juce::jmap(y, 0.0f, 1.0f, 0.0f, offset);
            }

            float yPos = midY + sumY;
            sumPath.lineTo((float)x, yPos);
            sumPoints.push_back({(float)x, yPos});
            // updateSampledCurve();
        }

        g.setColour(juce::Colours::yellow);
        g.strokePath(sumPath, juce::PathStrokeType(2.0f));
        for(auto &gauss : gaussians) {
            float mean = gauss.peakX * bounds.getWidth();
            auto peakPixelX = mean;
            auto peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;
            g.setColour(gauss.colour);
            g.fillEllipse(peakPixelX - 5, peakPixelY - 5, 20, 20);
        }

        if(showSumHoverPoint) {
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(hoverSumPoint.x - 5, hoverSumPoint.y - 5, 15, 15);
        }
    }

    void mouseDown(const juce::MouseEvent &e) override {
        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();
        bool clickedOnPeak = false;

        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &gauss = gaussians[i];
            auto peakPixelX = gauss.peakX * bounds.getWidth();
            auto peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;

            if(juce::Point<float>(peakPixelX, peakPixelY).getDistanceFrom(e.position) < 10.0f) {
                clickedOnPeak = true;
                draggingIndex = (int)i;
                initialMouseX = e.position.x;
                initialSigma = gauss.sigmaNorm;
                break;
            }
        }

        auto now = juce::Time::getCurrentTime();
        if(!clickedOnPeak && (now.toMilliseconds() - lastClickTime.toMilliseconds() < 400)
           && lastClickPos.getDistanceFrom(e.position) < 5.0f) {
            float px = e.position.x / bounds.getWidth();
            float py = (e.position.y - midY) / (bounds.getHeight() * 0.5f);

            gaussians.push_back({px, py, initialSigma, colours[colourIndex]});
            colourIndex = (colourIndex + 1) % (int)colours.size();

            draggingIndex = (int)gaussians.size() - 1;
            initialMouseX = e.position.x;
            initialSigma = 0.15f;
        }

        lastClickTime = now;
        lastClickPos = e.position;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        if(draggingIndex >= 0 && draggingIndex < (int)gaussians.size()) {
            auto &gauss = gaussians[(size_t)draggingIndex];
            auto bounds = getLocalBounds().toFloat();
            auto midY = bounds.getCentreY();

            if(e.mods.isCommandDown()) {
                float deltaX = (e.position.x - initialMouseX) / bounds.getWidth();
                gauss.sigmaNorm = juce::jlimit(0.005f, 0.5f, initialSigma + deltaX);
            } else {
                gauss.peakX = juce::jlimit(0.0f, 1.0f, e.position.x / bounds.getWidth());
                gauss.peakY
                 = juce::jlimit(-1.0f, 1.0f, (e.position.y - midY) / (bounds.getHeight() * 0.5f));
            }
        }
    }

    void mouseUp(const juce::MouseEvent &) override { draggingIndex = -1; }

    void mouseDoubleClick(const juce::MouseEvent &e) override {
        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();

        for(size_t i = 0; i < gaussians.size(); ++i) {
            auto &gauss = gaussians[i];
            auto peakPixelX = gauss.peakX * bounds.getWidth();
            auto peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;

            if(juce::Point<float>(peakPixelX, peakPixelY).getDistanceFrom(e.position) < 10.0f) {
                gaussians.erase(gaussians.begin() + (int)i);
                repaint();
                return;
            }
        }

        float px = e.position.x / bounds.getWidth();
        float py = (e.position.y - midY) / (bounds.getHeight() * 0.5f);
        addGaussian(px, py, initialSigma);
    }

    void mouseMove(const juce::MouseEvent &e) override {
        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();

        for(auto &gauss : gaussians) {
            auto peakPixelX = gauss.peakX * bounds.getWidth();
            auto peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;

            if(juce::Point<float>(peakPixelX, peakPixelY).getDistanceFrom(e.position) < 30.0f) {
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
                showSumHoverPoint = false;
                return;
            }
        }

        const float threshold = 8.0f;
        for(auto &p : sumPoints) {
            if(std::abs(p.x - e.position.x) < 1.0f && std::abs(p.y - e.position.y) < threshold) {
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
                hoverSumPoint = {e.position.x, p.y};
                showSumHoverPoint = true;
                return;
            }
        }

        setMouseCursor(juce::MouseCursor::NormalCursor);
        showSumHoverPoint = false;
    }

    void mouseExit(const juce::MouseEvent &) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        showSumHoverPoint = false;
        repaint();
    }

  private:
    struct Gaussian {
        float peakX;
        float peakY;
        float sigmaNorm;
        juce::Colour colour;
    };
    void updateSampledCurve() {
        for(size_t i = 0; i < Parameters::FFT_SIZE; ++i) {
            float normalizedX = (float)i / (float)(Parameters::FFT_SIZE - 1);

            float sumY = 0.0f;

            for(auto &gauss : gaussians) {
                float dx = normalizedX - gauss.peakX;
                float sigma = gauss.sigmaNorm;
                float gaussValue = std::exp(-0.5f * (dx * dx) / (sigma * sigma));

                sumY += gaussValue * gauss.peakY;
            }

            sampledCurve[i] = sumY;
        }

        for(auto i : sampledCurve) {
            DBG(i);
        }
    }

    juce::Time lastClickTime;
    juce::Point<float> lastClickPos;
    std::vector<Gaussian> gaussians;
    int draggingIndex = -1;

    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;

    std::vector<juce::Point<float>> sumPoints;
    juce::Point<float> hoverSumPoint{0, 0};
    bool showSumHoverPoint = false;
    std::array<float, Parameters::FFT_SIZE> sampledCurve = {0.0f};

    std::array<juce::Colour, 5> colours
     = {juce::Colours::red, juce::Colours::green, juce::Colours::blue, juce::Colours::orange,
        juce::Colours::purple};
    int colourIndex = 0;
};
