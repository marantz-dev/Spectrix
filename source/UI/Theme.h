
#pragma once
#include <JuceHeader.h>

class Theme : public juce::LookAndFeel_V4 {
  public:
    void
    drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                     float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) override {
        auto bounds = juce::Rectangle<float>(x, y, width, height);
        auto centre = bounds.getCentre();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 8.0f;

        auto knobColor = juce::Colour(0xffe8e8e8);
        auto knobShadow = juce::Colour(0xffc0c0c0);
        auto knobHighlight = juce::Colour(0xfff5f5f5);
        auto markColor = juce::Colours::grey.brighter();
        auto pointerColor = juce::Colour(0xff888888);

        // ########## TICKMARKS ##########

        auto tickRadius = radius - 3.0f;
        auto numTicks = 11;
        for(int i = 0; i < numTicks; ++i) {
            auto tickAngle
             = rotaryStartAngle + (float)i / (numTicks - 1) * (rotaryEndAngle - rotaryStartAngle);
            auto tickLength = (i % 2 == 0) ? 6.0f : 3.0f;
            auto tickThickness = (i % 2 == 0) ? 1.2f : 0.8f;

            auto innerX = centre.x
                          + std::cos(tickAngle - juce::MathConstants<float>::halfPi)
                             * (tickRadius - tickLength);
            auto innerY = centre.y
                          + std::sin(tickAngle - juce::MathConstants<float>::halfPi)
                             * (tickRadius - tickLength);
            auto outerX
             = centre.x + std::cos(tickAngle - juce::MathConstants<float>::halfPi) * tickRadius;
            auto outerY
             = centre.y + std::sin(tickAngle - juce::MathConstants<float>::halfPi) * tickRadius;

            g.setColour(markColor.withAlpha(0.4f));
            g.drawLine(innerX, innerY, outerX, outerY, tickThickness);
        }

        // ########## MAIN KNOB ##########

        auto knobRadius = radius - 15.0f;
        juce::ColourGradient knobGradient(
         knobHighlight, centre.x - knobRadius * 0.4f, centre.y - knobRadius * 0.4f,
         knobColor.darker(0.1f), centre.x + knobRadius * 0.6f, centre.y + knobRadius * 0.6f, true);
        g.setGradientFill(knobGradient);
        g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f,
                      knobRadius * 2.0f);

        g.setColour(knobShadow.withAlpha(0.3f));
        g.drawEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f,
                      knobRadius * 2.0f, 1.0f);

        // ########## POINTER ##########

        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto pointerLength = knobRadius * 0.95f;
        auto pointerThickness = 2.5f;

        auto pointerX
         = centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLength;
        auto pointerY
         = centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLength;

        g.setColour(pointerColor);
        g.drawLine(centre.x, centre.y, pointerX, pointerY, pointerThickness);

        auto centerDotRadius = 1.5f;
        g.setColour(pointerColor.withAlpha(0.6f));
        g.fillEllipse(centre.x - centerDotRadius, centre.y - centerDotRadius,
                      centerDotRadius * 2.0f, centerDotRadius * 2.0f);
    }

    // ###################
    // #                 #
    // #  TOGGLE BUTTON  #
    // #                 #
    // ###################

    void
    drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                     bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto isToggleOn = button.getToggleState();
        auto isButtonDown = shouldDrawButtonAsDown;
        auto isHovered = shouldDrawButtonAsHighlighted;
        auto isEnabled = button.isEnabled();

        auto backgroundColor = juce::Colour(0xff3a3a3a);
        auto onColor = juce::Colour(0xffff9500);
        auto offColor = juce::Colour(0xff464646);
        auto hoverColor = juce::Colour(0xff525252);
        auto textOffColor = juce::Colour(0xffcccccc);
        auto textOnColor = juce::Colour(0xff000000);

        juce::Colour buttonColor;
        if(isToggleOn) {
            buttonColor = isButtonDown ? onColor.darker(0.15f) : onColor;
        } else {
            if(isButtonDown) {
                buttonColor = offColor.darker(0.1f);
            } else if(isHovered) {
                buttonColor = hoverColor;
            } else {
                buttonColor = offColor;
            }
        }
        if(!isEnabled) {
            buttonColor = juce::Colour(0xff3a3a3a).withAlpha(0.5f);
            textOffColor = textOffColor.withAlpha(0.5f);
            textOnColor = textOnColor.withAlpha(0.5f);
        }

        auto cornerRadius = 2.0f;

        g.setColour(buttonColor);
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(backgroundColor.darker(0.2f).withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, cornerRadius, 0.5f);

        if(button.getButtonText().isNotEmpty()) {
            g.setColour(isToggleOn ? textOnColor : textOffColor);
            g.setFont(juce::FontOptions(bounds.getHeight() * 0.5f, juce::Font::plain));
            // g.setFont(juce::Font(bounds.getHeight() * 0.5f, juce::Font::plain));
            g.drawFittedText(button.getButtonText(), bounds.toNearestInt(),
                             juce::Justification::centred, 1);
        }
    }

    // ##########################################
    // #                                        #
    // #  MAKE CURSOR GO KAPUT WHEN PRESSED :)  #
    // #                                        #
    // ##########################################

    juce::MouseCursor getMouseCursorFor(juce::Component &component) override {
        if(auto *slider = dynamic_cast<juce::Slider *>(&component)) {
            if(slider->isMouseButtonDown())
                return juce::MouseCursor::NoCursor;
            else
                return juce::MouseCursor::PointingHandCursor;
        }
        return juce::LookAndFeel_V4::getMouseCursorFor(component);
    }
};
