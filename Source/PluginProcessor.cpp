#include "PluginProcessor.h"
#include "PluginEditor.h"

WetDiaperAudioProcessor::WetDiaperAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    rebuildLUT();
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
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("asymmetric", 1), "Asymmetric", false));

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
    rmsWindowSize_ = juce::jmax(1, static_cast<int>(sampleRate / 60.0));
    rmsSum_ = 0.0f;
    rmsSampleCount_ = 0;
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

    {
        const float* in = buffer.getReadPointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            rmsSum_ += in[i] * in[i];
            if (++rmsSampleCount_ >= rmsWindowSize_)
            {
                inputLevelRms.store(std::sqrt(rmsSum_ / static_cast<float>(rmsSampleCount_)));
                rmsSum_ = 0.0f;
                rmsSampleCount_ = 0;
            }
        }
    }

    if (apvts.getRawParameterValue("bypass")->load() >= 0.5f)
    {
        if (numCh >= 2)
            buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        return;
    }

    const float drive  = apvts.getRawParameterValue("drive")->load();
    const float tone   = apvts.getRawParameterValue("tone")->load();
    const float volume = apvts.getRawParameterValue("volume")->load();

    int readIdx = activeLutIndex_.load(std::memory_order_acquire);
    bool symmetric = apvts.getRawParameterValue("asymmetric")->load() < 0.5f;
    const float* leftLut = symmetric ? lutBuffers_[readIdx] : leftLutBuffers_[readIdx];
    for (auto& d : drives)
    {
        d.setDrive(drive);
        d.setTone(tone);
        d.setVolume(volume);
        d.setLUT(lutBuffers_[readIdx], leftLut, BezierCurve::kLutSize);
    }

    drives[0].processMono(buffer.getWritePointer(0), numSamples);
    for (int ch = 1; ch < numCh; ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
}

juce::AudioProcessorEditor* WetDiaperAudioProcessor::createEditor()
{
    return new WetDiaperAudioProcessorEditor(*this);
}

bool WetDiaperAudioProcessor::hasEditor() const { return true; }

void WetDiaperAudioProcessor::rebuildLUT()
{
    int current = activeLutIndex_.load(std::memory_order_acquire);
    int writeIdx = 1 - current;
    bezierCurve_.generateLUT(lutBuffers_[writeIdx]);
    leftBezierCurve_.generateLUT(leftLutBuffers_[writeIdx]);
    activeLutIndex_.store(writeIdx, std::memory_order_release);
    curveVersion_.fetch_add(1, std::memory_order_relaxed);
}

static juce::ValueTree serializeCurve(const BezierCurve& curve, const juce::Identifier& name)
{
    juce::ValueTree tree(name);

    juce::ValueTree startH("StartHandle");
    auto sh = curve.getStartOutHandle();
    startH.setProperty("outDx", sh.dx, nullptr);
    startH.setProperty("outDy", sh.dy, nullptr);
    tree.addChild(startH, -1, nullptr);

    for (int i = 0; i < curve.getNumPoints(); ++i)
    {
        auto& pt = curve.getPoint(i);
        juce::ValueTree ptTree("Point");
        ptTree.setProperty("x", pt.x, nullptr);
        ptTree.setProperty("y", pt.y, nullptr);
        ptTree.setProperty("inDx", pt.in.dx, nullptr);
        ptTree.setProperty("inDy", pt.in.dy, nullptr);
        ptTree.setProperty("outDx", pt.out.dx, nullptr);
        ptTree.setProperty("outDy", pt.out.dy, nullptr);
        tree.addChild(ptTree, -1, nullptr);
    }

    juce::ValueTree endH("EndHandle");
    auto eh = curve.getEndInHandle();
    endH.setProperty("inDx", eh.dx, nullptr);
    endH.setProperty("inDy", eh.dy, nullptr);
    tree.addChild(endH, -1, nullptr);

    return tree;
}

static void deserializeCurve(BezierCurve& curve, const juce::ValueTree& child)
{
    curve.reset();

    auto startH = child.getChildWithName("StartHandle");
    if (startH.isValid())
        curve.moveStartOutHandle(
            static_cast<float>(startH.getProperty("outDx", 1.0f / 3.0f)),
            static_cast<float>(startH.getProperty("outDy", 1.0f / 3.0f)));

    auto endH = child.getChildWithName("EndHandle");
    if (endH.isValid())
        curve.moveEndInHandle(
            static_cast<float>(endH.getProperty("inDx", -1.0f / 3.0f)),
            static_cast<float>(endH.getProperty("inDy", -1.0f / 3.0f)));

    for (int i = 0; i < child.getNumChildren(); ++i)
    {
        auto ch = child.getChild(i);
        if (ch.hasType("Point"))
        {
            float px = static_cast<float>(ch.getProperty("x", 0.5f));
            float py = static_cast<float>(ch.getProperty("y", 0.5f));
            int idx = curve.addPoint(px, py);
            if (idx >= 0)
            {
                curve.moveInHandle(idx,
                    static_cast<float>(ch.getProperty("inDx", 0.0f)),
                    static_cast<float>(ch.getProperty("inDy", 0.0f)));
                curve.moveOutHandle(idx,
                    static_cast<float>(ch.getProperty("outDx", 0.0f)),
                    static_cast<float>(ch.getProperty("outDy", 0.0f)));
            }
        }
    }
}

void WetDiaperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.addChild(serializeCurve(bezierCurve_, "BezierCurve"), -1, nullptr);
    state.addChild(serializeCurve(leftBezierCurve_, "LeftBezierCurve"), -1, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void WetDiaperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xml);

        auto bezierChild = tree.getChildWithName("BezierCurve");
        if (bezierChild.isValid())
        {
            deserializeCurve(bezierCurve_, bezierChild);
            tree.removeChild(bezierChild, nullptr);
        }

        auto leftChild = tree.getChildWithName("LeftBezierCurve");
        if (leftChild.isValid())
        {
            deserializeCurve(leftBezierCurve_, leftChild);
            tree.removeChild(leftChild, nullptr);
        }

        rebuildLUT();
        apvts.replaceState(tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WetDiaperAudioProcessor();
}
