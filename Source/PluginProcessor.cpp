#include "PluginProcessor.h"
#include "PluginEditor.h"

WetDiaperAudioProcessor::WetDiaperAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

WetDiaperAudioProcessor::~WetDiaperAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout WetDiaperAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drive", 1), "Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 0.3f), 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tone", 1), "Tone",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("volume", 1), "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));

    return { params.begin(), params.end() };
}

const juce::String WetDiaperAudioProcessor::getName() const { return JucePlugin_Name; }
bool WetDiaperAudioProcessor::acceptsMidi() const { return false; }
bool WetDiaperAudioProcessor::producesMidi() const { return false; }
bool WetDiaperAudioProcessor::isMidiEffect() const { return false; }
double WetDiaperAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int WetDiaperAudioProcessor::getNumPrograms() { return 1; }
int WetDiaperAudioProcessor::getCurrentProgram() { return 0; }
void WetDiaperAudioProcessor::setCurrentProgram(int) {}
const juce::String WetDiaperAudioProcessor::getProgramName(int) { return {}; }
void WetDiaperAudioProcessor::changeProgramName(int, const juce::String&) {}

void WetDiaperAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    for (auto& d : drives) d.prepare(sampleRate, samplesPerBlock);
    setLatencySamples(0);
}

void WetDiaperAudioProcessor::releaseResources()
{
    for (auto& d : drives) d.reset();
}

bool WetDiaperAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() &&
        mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != juce::AudioChannelSet::mono() &&
        mainIn != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void WetDiaperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (apvts.getRawParameterValue("bypass")->load() >= 0.5f)
    {
        if (numCh >= 2)
            buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        return;
    }

    const float drive  = apvts.getRawParameterValue("drive")->load();
    const float tone   = apvts.getRawParameterValue("tone")->load();
    const float volume = apvts.getRawParameterValue("volume")->load();

    for (auto& d : drives) { d.setDrive(drive); d.setTone(tone); d.setVolume(volume); }

    drives[0].processMono(buffer.getWritePointer(0), numSamples);
    for (int ch = 1; ch < numCh; ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
}

juce::AudioProcessorEditor* WetDiaperAudioProcessor::createEditor()
{
    return new WetDiaperAudioProcessorEditor(*this);
}

bool WetDiaperAudioProcessor::hasEditor() const { return true; }

void WetDiaperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void WetDiaperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WetDiaperAudioProcessor();
}
