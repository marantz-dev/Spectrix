
#pragma once

#include "PluginParameters.h"

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
