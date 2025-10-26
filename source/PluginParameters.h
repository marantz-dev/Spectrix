#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include <JuceHeader.h>

namespace Parameters {
    static const int FFT_SIZE = 4096;

    // Parameter IDs
    static const String curveShiftDBID = "CS";
    static const String attackTimeID = "AT";
    static const String releaseTimeID = "RT";
    static const String kneeWidthID = "KW";
    static const String compressorModeID = "IC";
    static const String inputGainID = "IG";
    static const String outputGainID = "OG";
    static const String ratioID = "RA";

    // Default values
    static const float defaultCurveShiftDB = -10.0f;
    static const float defaultAttackTime = 10.0f;   // ms
    static const float defaultReleaseTime = 100.0f; // ms
    static const float defaultKneeWidth = 3.0f;     // dB
    static const bool defaultCompressorMode = false;
    static const float defaultInputGain = 0.0f;  // dB
    static const float defaultOutputGain = 0.0f; // dB
    static const float defaultRatio = 4.0f;

    // MIN MAX BOUNDS
    static const float minAttack = 1.0f;
    static const float maxAttack = 1000.0f;
    static const float minRelease = 10.0f;
    static const float maxRelease = 1000.0f;
    static const float minCurveShift = -96.0f;
    static const float maxCurveShift = 12.0f;
    static const float minRatio = 1.0f;
    static const float maxRatio = 20.0f;
    static const float minKnee = 0.0f;
    static const float maxKnee = 12.0f;
    static const float minInputGain = -24.0f;
    static const float maxInputGain = 24.0f;
    static const float minOutputGain = -24.0f;
    static const float maxOutputGain = 24.0f;

    // SKEW & STEP SIZE
    static const float stepSizeAttack = 1.0f;
    static const float skewFactorAttack = 0.5f;
    static const float stepSizeRelease = 1.0f;
    static const float skewFactorRelease = 0.3f;
    static const float stepSizeCurveShift = 0.1f;
    static const float skewFactorCurveShift = 1.0f;
    static const float stepSizeRatio = 0.1f;
    static const float skewFactorRatio = 0.5f;
    static const float stepSizeKnee = 0.1f;
    static const float skewFactorKnee = 1.0f;
    static const float stepSizeInputGain = 0.1f;
    static const float skewFactorInputGain = 1.0f;
    static const float stepSizeOutputGain = 0.1f;
    static const float skewFactorOutputGain = 1.0f;

    // Visualizer constants
    static const float minDBVisualizer = -96.0f;
    static const float maxDBVisualizer = 12.0f;
    static const float warpMidPoint = 0.4f;
    static const float warpSteepness = 0.0f;

    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        int id = 0;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        // Response Curve Shift
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("CS", id++), "Response Curve Shift",
         NormalisableRange<float>(-96.0f, 12.0f, 0.1f, 1.0f), defaultCurveShiftDB));

        // Attack Time
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("AT", id++), "Attack", NormalisableRange<float>(1.0f, 1000.0f, 1.0f, 0.5f),
         defaultAttackTime));

        // Release Time
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("RT", id++), "Release", NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.3f),
         defaultReleaseTime));

        // Ratio
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("RA", id++), "Ratio", NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f),
         defaultRatio));

        // Knee Width
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("KW", id++), "Knee", NormalisableRange<float>(0.0f, 12.0f, 0.1f, 1.0f),
         defaultKneeWidth));

        // Compressor MODE
        params.push_back(std::make_unique<AudioParameterChoice>(
         ParameterID(compressorModeID, id++), "Compressor Mode",
         StringArray{"Compressor", "Clipper", "Gate"}, 0));

        // Input Gain
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("IG", id++), "Input Gain", NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f),
         defaultInputGain));

        // Output Gain
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("OG", id++), "Output Gain",
         NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f), defaultOutputGain));

        return {params.begin(), params.end()};
    }

    static void addListeners(AudioProcessorValueTreeState &valueTreeState,
                             AudioProcessorValueTreeState::Listener *listener) {
        std::unique_ptr<XmlElement> xml(valueTreeState.copyState().createXml());
        for(auto *element : xml->getChildWithTagNameIterator("PARAM")) {
            const String &id = element->getStringAttribute("id");
            valueTreeState.addParameterListener(id, listener);
        }
    }
} // namespace Parameters
