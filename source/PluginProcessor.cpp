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
    setLatencySamples(Parameters::FFT_SIZE - 1);
    inputGain.reset(sampleRate, 0.05);
    inputGain.setCurrentAndTargetValue(1.0);
    outputGain.reset(sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue(1.0);
}

void SpectrixAudioProcessor::releaseResources() {}

void SpectrixAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numCh = buffer.getNumChannels();
    auto bufferData = buffer.getArrayOfWritePointers();

    for(int ch = 0; ch < numCh; ++ch)
        inputGain.applyGain(bufferData[ch], buffer.getNumSamples());

    spectralCompressor.processBlock(buffer);

    for(int ch = 0; ch < numCh; ++ch)
        inputGain.applyGain(bufferData[ch], buffer.getNumSamples());
}

bool SpectrixAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *SpectrixAudioProcessor::createEditor() {
    return new SpectrixAudioProcessorEditor(*this, parameters);
}

void SpectrixAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto state = parameters.copyState();
    state.addChild(responseCurve.toValueTree(), -1, nullptr);
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpectrixAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if(xmlState != nullptr) {
        auto vt = juce::ValueTree::fromXml(*xmlState);
        if(vt.hasType(parameters.state.getType())) {
            parameters.replaceState(vt);
            if(auto curveTree = vt.getChildWithName("GaussianResponse"); curveTree.isValid()) {
                responseCurve.fromValueTree(curveTree);
            }
        }
    }
}
bool SpectrixAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
    if(layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
       && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    if(layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return true;
}

void SpectrixAudioProcessor::parameterChanged(const String &paramID, float newValue) {
    if(paramID == Parameters::curveShiftDBID) {
        responseCurve.setResponseCurveShiftDB(newValue);
    }

    if(paramID == Parameters::attackTimeID) {
        spectralCompressor.setAttackTime(newValue);
    }

    if(paramID == Parameters::releaseTimeID) {
        spectralCompressor.setReleaseTime(newValue);
    }

    if(paramID == Parameters::ratioID) {
        spectralCompressor.setRatio(newValue);
    }

    if(paramID == Parameters::kneeWidthID) {
        spectralCompressor.setKnee(newValue);
    }

    if(paramID == Parameters::compressorModeID) {
        spectralCompressor.setCompressorMode(newValue);
    }

    if(paramID == Parameters::inputGainID) {
        inputGain.setTargetValue(juce::Decibels::decibelsToGain(newValue));
    }

    if(paramID == Parameters::outputGainID) {
        outputGain.setTargetValue(juce::Decibels::decibelsToGain(newValue));
    }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SpectrixAudioProcessor(); }
