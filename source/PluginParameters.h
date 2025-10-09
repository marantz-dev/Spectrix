#pragma once

#include <JuceHeader.h>

namespace Parameters {
    static const int FFT_SIZE = 4096;

    static const String magThreshold = "MT";
    static const String spectrumAttack = "SA";

    static const float defaultThresholdValue = 0.8f;
    static const float defaultSpectrumDetail = 0.6f;

    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        int id = 0;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("MT", id++), "Magnitude Threshold",
         NormalisableRange<float>(0.001f, 1.0f, 0.001, 1.0), defaultThresholdValue));

        params.push_back(std::make_unique<AudioParameterFloat>(
         ParameterID("SA", id++), "Spectrum Detail",
         NormalisableRange<float>(0.001f, 1.0f, 0.001, 1.0), defaultSpectrumDetail));

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
