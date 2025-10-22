#pragma once
#include <JuceHeader.h>

namespace Parameters {
    static const int FFT_SIZE = 4096;

    // Parameter IDs
    static const String magThreshold = "MT";
    static const String spectrumAttack = "SA";
    static const String responseCurveShiftDBID = "CS";
    static const String attackTimeID = "AT";
    static const String releaseTimeID = "RT";
    static const String kneeWidthID = "KW";
    static const String isClipperID = "IC";
    static const String inputGainID = "IG";
    static const String outputGainID = "OG";
    static const String ratioID = "RA";

    // Default values
    static const float defaultThresholdValue = 0.8f;
    static const float defaultSpectrumDetail = 0.6f;
    static const float responseCurveShiftDB = -10.0f;
    static const float defaultAttackTime = 10.0f;   // ms
    static const float defaultReleaseTime = 100.0f; // ms
    static const float defaultKneeWidth = 3.0f;     // dB
    static const bool defaultIsClipper = false;
    static const float defaultInputGain = 0.0f;  // dB
    static const float defaultOutputGain = 0.0f; // dB
    static const float defaultRatio = 4.0f;

    // Visualizer constants
    static const float minDBVisualizer = -80.0f;
    static const float maxDBVisualizer = 6.0f;
    static const float warpMidPoint = 0.4f;
    static const float warpSteepness = 0.0f;

    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        int id = 0;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        // Magnitude Threshold
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("MT", id++), "Magnitude Threshold",
         NormalisableRange<float>(0.001f, 1.0f, 0.001f, 1.0f), defaultThresholdValue));

        // Response Curve Shift
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("CS", id++), "Response Curve Shift",
         NormalisableRange<float>(-80.0f, 0.0f, 0.1f, 1.0f), responseCurveShiftDB));

        // Spectrum Detail
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("SA", id++), "Spectrum Detail",
         NormalisableRange<float>(0.001f, 1.0f, 0.001f, 1.0f), defaultSpectrumDetail));

        // Attack Time
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("AT", id++), "Attack", NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.3f),
         defaultAttackTime, String(), AudioProcessorParameter::genericParameter,
         [](float value, int) { return String(value, 1) + " ms"; },
         [](const String &text) { return text.dropLastCharacters(3).getFloatValue(); }));

        // Release Time
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("RT", id++), "Release", NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.3f),
         defaultReleaseTime, String(), AudioProcessorParameter::genericParameter,
         [](float value, int) { return String(value, 1) + " ms"; },
         [](const String &text) { return text.dropLastCharacters(3).getFloatValue(); }));

        // Ratio
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("RA", id++), "Ratio", NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f),
         defaultRatio, String(), AudioProcessorParameter::genericParameter,
         [](float value, int) {
             if(value >= 19.9f)
                 return String("Inf:1");
             return String(value, 1) + ":1";
         },
         [](const String &text) {
             if(text.startsWith("Inf"))
                 return 20.0f;
             return text.upToFirstOccurrenceOf(":", false, false).getFloatValue();
         }));

        // Knee Width
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("KW", id++), "Knee", NormalisableRange<float>(0.0f, 12.0f, 0.1f, 1.0f),
         defaultKneeWidth, String(), AudioProcessorParameter::genericParameter,
         [](float value, int) { return String(value, 1) + " dB"; },
         [](const String &text) { return text.dropLastCharacters(3).getFloatValue(); }));

        // Is Clipper (Bool)
        params.push_back(std::make_unique<AudioParameterBool>(ParameterID("IC", id++),
                                                              "Clipper Mode", defaultIsClipper));

        // Input Gain
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("IG", id++), "Input Gain", NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f),
         defaultInputGain, String(), AudioProcessorParameter::genericParameter,
         [](float value, int) { return (value >= 0 ? "+" : "") + String(value, 1) + " dB"; },
         [](const String &text) {
             return text.trimCharactersAtStart("+").dropLastCharacters(3).getFloatValue();
         }));

        // Output Gain
        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("OG", id++), "Output Gain",
         NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f), defaultOutputGain, String(),
         AudioProcessorParameter::genericParameter,
         [](float value, int) { return (value >= 0 ? "+" : "") + String(value, 1) + " dB"; },
         [](const String &text) {
             return text.trimCharactersAtStart("+").dropLastCharacters(3).getFloatValue();
         }));

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
