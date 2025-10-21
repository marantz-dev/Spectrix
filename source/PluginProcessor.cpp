#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

//==============================================================================
SpectrixAudioProcessor::SpectrixAudioProcessor()
    : AudioProcessor(BusesProperties()
                      .withInput("Input", AudioChannelSet::stereo(), true)
                      .withOutput("Output", AudioChannelSet::stereo(), true)),
      spectralCompressor(responseCurve),
      parameters(*this, nullptr, "PLG", Parameters::createParameterLayout())

{
    Parameters::addListeners(parameters, this);
}

SpectrixAudioProcessor::~SpectrixAudioProcessor() {}

//==============================================================================
void SpectrixAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    spectralCompressor.prepareToPlay(sampleRate);
}

void SpectrixAudioProcessor::releaseResources() {}

void SpectrixAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    spectralCompressor.processBlock(buffer);
}

bool SpectrixAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SpectrixAudioProcessor::createEditor() {
    return new SpectrixAudioProcessorEditor(*this);
}

void SpectrixAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpectrixAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if(xmlState.get() != nullptr)
        if(xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(ValueTree::fromXml(*xmlState));
}
bool SpectrixAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
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

void SpectrixAudioProcessor::parameterChanged(const String &paramID, float newValue) {
    if(paramID == Parameters::magThreshold) {}
    if(paramID == Parameters::spectrumAttack) {}
    if(paramID == Parameters::responseCurveShiftDBID) {
        responseCurve.setResponseCurveShiftDB(newValue);
    }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SpectrixAudioProcessor(); }
