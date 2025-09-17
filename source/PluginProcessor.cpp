#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

//==============================================================================
FFTProcessorAudioProcessor::FFTProcessorAudioProcessor()
    : AudioProcessor(BusesProperties()
                      .withInput("Input", AudioChannelSet::stereo(), true)
                      .withOutput("Output", AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PLG", Parameters::createParameterLayout())

{
  Parameters::addListeners(parameters, this);
}

FFTProcessorAudioProcessor::~FFTProcessorAudioProcessor() {}

//==============================================================================
void FFTProcessorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {}

void FFTProcessorAudioProcessor::releaseResources() {}

void FFTProcessorAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                            juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
}

bool FFTProcessorAudioProcessor::hasEditor() const {
  return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *FFTProcessorAudioProcessor::createEditor() {
  return new FFTProcessorAudioProcessorEditor(*this);
}

void FFTProcessorAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = parameters.copyState();
  std::unique_ptr<XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void FFTProcessorAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
  std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if(xmlState.get() != nullptr)
    if(xmlState->hasTagName(parameters.state.getType()))
      parameters.replaceState(ValueTree::fromXml(*xmlState));
}
bool FFTProcessorAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
  if(layouts.getMainInputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::mono())
    return false;

  if(layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
    return false;

  if(layouts.inputBuses[1] != AudioChannelSet::mono()
     && layouts.inputBuses[1] != AudioChannelSet::stereo()
     && layouts.inputBuses[1] != AudioChannelSet::disabled())
    return false;

  return true;
}

void FFTProcessorAudioProcessor::parameterChanged(const String &paramID, float newValue) {}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new FFTProcessorAudioProcessor(); }
