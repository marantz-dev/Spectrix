
#include "juce_graphics/juce_graphics.h"
#include <JuceHeader.h>
#include <array>
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
        // g.fillAll(juce::Colours::white);

        auto midY = bounds.getCentreY();

        // ---- Draw Gaussians ----
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

            // Peak marker
        }

        // ---- Draw SUM of Gaussians ----
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
        }

        g.setColour(juce::Colours::yellow);
        g.strokePath(sumPath, juce::PathStrokeType(2.0f));
        for(auto &gauss : gaussians) {
            float mean = gauss.peakX * bounds.getWidth();
            auto peakPixelX = mean;
            auto peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;
            // g.setColour(juce::Colours::white);
            // g.fillEllipse(peakPixelX - 5, peakPixelY - 5, 25, 25);
            g.setColour(gauss.colour);
            g.fillEllipse(peakPixelX - 5, peakPixelY - 5, 20, 20);
        }

        // If hovering near the sum line → draw marker
        if(showSumHoverPoint) {
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(hoverSumPoint.x - 10, hoverSumPoint.y - 10, 10, 10);
        }
    }

    void mouseDown(const juce::MouseEvent &e) override {
        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();
        bool clickedOnPeak = false;

        // Check peaks first
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

        // Timing logic for double click
        auto now = juce::Time::getCurrentTime();
        if(!clickedOnPeak && (now.toMilliseconds() - lastClickTime.toMilliseconds() < 400)
           && lastClickPos.getDistanceFrom(e.position) < 5.0f) {
            // Treat as double click → add new Gaussian
            float px = e.position.x / bounds.getWidth();
            float py = (e.position.y - midY) / (bounds.getHeight() * 0.5f);

            gaussians.push_back({px, py, initialSigma, colours[colourIndex]});
            colourIndex = (colourIndex + 1) % (int)colours.size();

            // Immediately drag the new Gaussian
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
                // Relative sigma adjustment
                float deltaX = (e.position.x - initialMouseX) / bounds.getWidth();
                gauss.sigmaNorm = juce::jlimit(0.005f, 0.5f, initialSigma + deltaX);
            } else {
                gauss.peakX = juce::jlimit(0.0f, 1.0f, e.position.x / bounds.getWidth());
                gauss.peakY
                 = juce::jlimit(-1.0f, 1.0f, (e.position.y - midY) / (bounds.getHeight() * 0.5f));
            }

            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent &) override { draggingIndex = -1; }

    void mouseDoubleClick(const juce::MouseEvent &e) override {
        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();

        // Check if double-clicked on an existing peak → delete
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

        // Otherwise add a new Gaussian
        float px = e.position.x / bounds.getWidth();
        float py = (e.position.y - midY) / (bounds.getHeight() * 0.5f);
        addGaussian(px, py, initialSigma);
    }

    void mouseMove(const juce::MouseEvent &e) override {
        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();

        // Check peaks first
        for(auto &gauss : gaussians) {
            auto peakPixelX = gauss.peakX * bounds.getWidth();
            auto peakPixelY = midY + gauss.peakY * bounds.getHeight() * 0.5f;

            if(juce::Point<float>(peakPixelX, peakPixelY).getDistanceFrom(e.position) < 10.0f) {
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
                showSumHoverPoint = false;
                repaint();
                return;
            }
        }

        // Check sum line
        const float threshold = 8.0f;
        for(auto &p : sumPoints) {
            if(std::abs(p.x - e.position.x) < 1.0f && std::abs(p.y - e.position.y) < threshold) {
                setMouseCursor(juce::MouseCursor::PointingHandCursor);
                hoverSumPoint = {e.position.x, p.y};
                showSumHoverPoint = true;
                repaint();
                return;
            }
        }

        // Default
        setMouseCursor(juce::MouseCursor::NormalCursor);
        showSumHoverPoint = false;
        repaint();
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

    juce::Time lastClickTime;
    juce::Point<float> lastClickPos;
    std::vector<Gaussian> gaussians;
    int draggingIndex = -1;

    // For sigma adjustment
    float initialMouseX = 0.0f;
    float initialSigma = 0.05f;

    // For sum hover detection
    std::vector<juce::Point<float>> sumPoints;
    juce::Point<float> hoverSumPoint{0, 0};
    bool showSumHoverPoint = false;

    std::array<juce::Colour, 5> colours
     = {juce::Colours::red, juce::Colours::green, juce::Colours::blue, juce::Colours::orange,
        juce::Colours::purple};
    int colourIndex = 0;
};
