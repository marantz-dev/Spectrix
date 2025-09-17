#pragma once

#include <JuceHeader.h>

namespace Parameters {
  static const String parameterName = "PARAMETERCODE";

  static const float defaultParameterValue = 0.5f;

  static AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterFloat>(
     ParameterID("PARAMETERCODE", 0), "PARAMETER DESCRIPTION",
     NormalisableRange<float>(0.0f, 1.0f, 0.001, 1.0), defaultParameterValue));
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
