#pragma once

#include <JuceHeader.h>

namespace Parameters {
      static const String parameterName = "PARAMETERCODE";
      static const String magThreshold = "MT";

      static const float defaultParameterValue = 0.5f;
      static const float defaultThresholdValue = 0.8f;

      // Makeup Gain
      static const String nameMakeup = "MAKEUP";
      static const float defaultMakeup = 0.0f;
      static const float minMakeup = -24.0f;
      static const float maxMakeup = 24.0f;
      static const float stepSizeMakeup = 0.1f;
      static const float skewFactorMakeup = 1.0f;

      static AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
            std::vector<std::unique_ptr<RangedAudioParameter>> params;

            params.push_back(std::make_unique<AudioParameterFloat>(
             ParameterID("PARAMETERCODE", 0), "PARAMETER DESCRIPTION",
             NormalisableRange<float>(0.0f, 1.0f, 0.001, 1.0), defaultParameterValue));

            params.push_back(std::make_unique<AudioParameterFloat>(
             ParameterID(nameMakeup, 1), "Makeup (dB)",
             NormalisableRange<float>(minMakeup, maxMakeup, stepSizeMakeup, skewFactorMakeup),
             defaultMakeup));

            params.push_back(std::make_unique<AudioParameterFloat>(
             ParameterID("MT", 2), "Magnitude Threshold",
             NormalisableRange<float>(0.001f, 1.0f, 0.001, 1.0), defaultThresholdValue));
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
