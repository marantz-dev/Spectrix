
#pragma once

#include "PluginParameters.h"
#include <JuceHeader.h>

namespace UIutils {

    inline void
    setupSlider(juce::Slider &slider, const juce::Slider::SliderStyle &style, float min, float max,
                float defaultValue, float stepSize, const juce::String &suffix, float skewFactor,
                juce::Label &label, const juce::String &labelText) {
        slider.setSliderStyle(style);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        slider.setRange(min, max, stepSize);
        slider.setValue(defaultValue);
        slider.setDoubleClickReturnValue(true, defaultValue);
        slider.setTextValueSuffix(suffix);
        slider.setSkewFactor(skewFactor);

        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
    }

    inline void attachLabel(juce::Label &label, juce::Slider *slider, int distance = 5) {
        label.attachToComponent(slider, false);
        label.setBounds(label.getX(), label.getY() + distance, label.getWidth(), label.getHeight());
    }

    inline void setupToggleButton(juce::ToggleButton &button, const juce::String &text) {
        button.setButtonText(text);
        button.setClickingTogglesState(true);
        button.setToggleState(false, juce::dontSendNotification);
    }

}
static const float minDB = Parameters::minDBVisualizer;
static const float maxDB = Parameters::maxDBVisualizer;

inline float DBWarp(float magnitudeDB) {
    float norm = juce::jmap(magnitudeDB, minDB, maxDB, 0.0f, 1.0f);
    float x = (norm - 0.44f) * 5.0f;
    float sigmoid = 1.0f / (1.0f + std::exp(-x));
    return juce::jmap(sigmoid, 0.0f, 1.0f, minDB, maxDB);
}
inline float inverseDBWarp(float yPos, juce::Rectangle<float> bounds) {
    float warpedDB = juce::jmap(yPos, bounds.getBottom(), bounds.getY(), minDB, maxDB);
    float sigmoid = juce::jmap(warpedDB, minDB, maxDB, 0.0f, 1.0f);
    sigmoid = juce::jlimit(0.001f, 0.999f, sigmoid);
    float x = -std::log(1.0f / sigmoid - 1.0f);
    float norm = (x / 5.0f) + 0.44f;
    norm = juce::jlimit(0.0f, 1.0f, norm);
    return juce::jmap(norm, 0.0f, 1.0f, minDB, maxDB);
}
