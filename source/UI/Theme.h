#pragma once
#include "juce_graphics/juce_graphics.h"
#include <JuceHeader.h>

class Theme : public juce::LookAndFeel_V4 {
  public:
    Theme() {
        // defined colors to match the screenshot vibe
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::grey);
        setColour(juce::Label::textColourId, juce::Colour(0xffe1e1e1)); // Brighter for the Name
    }

    // ###################
    // #                 #
    // #  ROTARY SLIDER  #
    // #                 #
    // ###################

    void
    drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                     float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) override {
        // 1. Setup Geometry
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();

        // 2. Define Colors
        // Track Background (Dark)
        auto trackColor = juce::Colour(0xff1d2328).brighter(0.1f);
        // Knob Face
        auto knobColor = juce::Colour(0xff15181d);

        // --- GRADIENT DEFINITION FOR ACTIVE ARC ---
        // Start Color (Top of knob - Bright Cyan)
        auto gradientTop = juce::Colour(0xff66fcf1);
        // End Color (Bottom of knob - Deep Electric Blue)
        auto gradientBottom = juce::Colour(0xff2d79eb);

        // 3. Draw Background Track (The "Empty" part)
        auto lineW = 3.5f;
        auto arcRadius = radius - lineW * 0.5f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                    rotaryStartAngle, rotaryEndAngle, true);

        g.setColour(trackColor);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

        // 4. Draw Active Track (The "Value" part) with GRADIENT
        if(slider.isEnabled()) {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                                   toAngle, true);

            // Create a vertical gradient spanning the height of the knob
            juce::ColourGradient activeGradient(
             gradientTop, centre.x, centre.y - radius,    // Start at Top
             gradientBottom, centre.x, centre.y + radius, // End at Bottom
             false                                        // Not radial
            );

            g.setGradientFill(activeGradient);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // 5. Draw Knob Face
        auto knobRadius = radius - 8.0f;

        g.setColour(knobColor);
        g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f,
                      knobRadius * 2.0f);

        // Subtle edge
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f,
                      knobRadius * 2.0f, 1.0f);

        // 6. Draw Indicator Dot
        // We keep the dot the bright cyan color so it pops against the gradient
        float dotDist = knobRadius * 0.75f;
        juce::Point<float> thumbPoint(
         centre.x + dotDist * std::cos(toAngle - juce::MathConstants<float>::halfPi),
         centre.y + dotDist * std::sin(toAngle - juce::MathConstants<float>::halfPi));

        float dotSize = 4.0f;
        g.setColour(gradientTop); // Use the bright active color
        g.fillEllipse(juce::Rectangle<float>(dotSize, dotSize).withCentre(thumbPoint));

        // Dot Glow
        g.setColour(gradientTop.withAlpha(0.4f));
        g.fillEllipse(
         juce::Rectangle<float>(dotSize * 2.5f, dotSize * 2.5f).withCentre(thumbPoint));
    }

    // ###################
    // #                 #
    // #   TEXT STYLING  #
    // #                 #
    // ###################

    // 1. Font for the Slider Value (e.g. "10.0 ms")
    juce::Font getLabelFont(juce::Label &label) override {
        return juce::Font("Roboto", 13.0f, juce::Font::plain); // Clean, sans-serif
    }

    // 2. Custom Text Box (Transparent, no borders)
    juce::Label *createSliderTextBox(juce::Slider &slider) override {
        auto *l = LookAndFeel_V4::createSliderTextBox(slider);

        // Remove the default white background and outline
        l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour(juce::Label::textColourId, juce::Colours::grey);

        return l;
    }

    // ###################
    // #                 #
    // #  TOGGLE BUTTON  #
    // #                 #
    // ###################

    // Inside Theme.h

    void
    drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                     bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();
        auto isOn = button.getToggleState();
        auto isHover = shouldDrawButtonAsHighlighted;

        // 1. Colors & Style
        auto activeColor = juce::Colour(0xff66fcf1);   // Cyan
        auto inactiveColor = juce::Colour(0xff35384a); // Dark Grey
        auto textColor = isOn ? juce::Colours::white : juce::Colours::grey;
        auto mainColor = isOn ? activeColor : inactiveColor;

        // 2. Layout Calculation
        auto h = bounds.getHeight();

        // A. Indicator Box (Left)
        auto boxSize = juce::jmin(h * 0.6f, 16.0f);
        auto boxBounds = juce::Rectangle<float>(boxSize, boxSize)
                          .withCentre({bounds.getX() + 15.0f, bounds.getCentreY()});

        // B. Symbol Area (Middle)
        auto iconWidth = 20.0f;
        auto iconBounds
         = juce::Rectangle<float>(boxBounds.getRight() + 8.0f, bounds.getY(), iconWidth, h);

        // C. Text Area (Right)
        auto textBounds
         = juce::Rectangle<float>(iconBounds.getRight() + 5.0f, bounds.getY(),
                                  bounds.getWidth() - iconBounds.getRight() - 5.0f, h);

        // 3. Draw Indicator Box (Glowy style)
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(boxBounds.translated(0, 1), 3.0f); // Shadow

        if(isOn) {
            // Glow
            g.setColour(activeColor.withAlpha(0.4f));
            g.fillRoundedRectangle(boxBounds.expanded(3.0f), 4.0f);

            // Lit Gradient
            juce::ColourGradient litGrad(activeColor.brighter(0.2f), boxBounds.getTopLeft(),
                                         activeColor.darker(0.1f), boxBounds.getBottomLeft(),
                                         false);
            g.setGradientFill(litGrad);
        } else {
            // Unlit Gradient
            juce::ColourGradient unlitGrad(inactiveColor.darker(0.2f), boxBounds.getTopLeft(),
                                           inactiveColor, boxBounds.getBottomLeft(), false);
            g.setGradientFill(unlitGrad);
        }
        g.fillRoundedRectangle(boxBounds, 2.0f);

        // Border
        g.setColour(isOn ? activeColor.brighter(0.4f) : juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(boxBounds, 2.0f, 1.0f);

        // 4. Draw Symbol based on Text
        g.setColour(isOn ? activeColor : juce::Colours::grey);

        juce::Path iconPath;
        auto iconCenter = iconBounds.getCentre();
        auto s = 6.0f; // Scale for icons

        juce::String text = button.getButtonText();

        if(text.containsIgnoreCase("Compressor")) {
            // curve going flat
            iconPath.startNewSubPath(iconCenter.x - s, iconCenter.y + s);
            iconPath.lineTo(iconCenter.x, iconCenter.y);
            iconPath.lineTo(iconCenter.x + s, iconCenter.y);
        } else if(text.containsIgnoreCase("Expander")) {
            // Arrows pointing out
            iconPath.startNewSubPath(iconCenter.x - s, iconCenter.y);
            iconPath.lineTo(iconCenter.x - 3, iconCenter.y - 3);
            iconPath.startNewSubPath(iconCenter.x - s, iconCenter.y);
            iconPath.lineTo(iconCenter.x - 3, iconCenter.y + 3);

            iconPath.startNewSubPath(iconCenter.x + s, iconCenter.y);
            iconPath.lineTo(iconCenter.x + 3, iconCenter.y - 3);
            iconPath.startNewSubPath(iconCenter.x + s, iconCenter.y);
            iconPath.lineTo(iconCenter.x + 3, iconCenter.y + 3);
        } else if(text.containsIgnoreCase("Clipper")) {
            // Square wave / Flat top
            iconPath.startNewSubPath(iconCenter.x - s, iconCenter.y + s);
            iconPath.lineTo(iconCenter.x - s / 2, iconCenter.y - s / 2);
            iconPath.lineTo(iconCenter.x + s / 2, iconCenter.y - s / 2); // The clip
            iconPath.lineTo(iconCenter.x + s, iconCenter.y + s);
        } else if(text.containsIgnoreCase("Gate")) {
            // Gate symbol (vertical line stopping signal)
            iconPath.startNewSubPath(iconCenter.x - s, iconCenter.y);
            iconPath.lineTo(iconCenter.x, iconCenter.y);
            iconPath.startNewSubPath(iconCenter.x, iconCenter.y - s);
            iconPath.lineTo(iconCenter.x, iconCenter.y + s);
        }

        g.strokePath(iconPath, juce::PathStrokeType(1.5f));

        // 5. Draw Text
        g.setColour(textColor);
        g.setFont(juce::Font("Roboto", 14.0f, juce::Font::plain));
        if(isHover)
            g.setColour(textColor.brighter(0.1f));

        g.drawFittedText(text, textBounds.toNearestInt(), juce::Justification::centredLeft, 1);
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
