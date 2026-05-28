#include "PluginProcessor.h"
#include "PluginEditor.h"

static void registerCurveListeners(juce::AudioProcessorValueTreeState& apvts,
                                    juce::AudioProcessorValueTreeState::Listener* listener,
                                    const juce::String& prefix)
{
    apvts.addParameterListener(prefix + "sh_dx", listener);
    apvts.addParameterListener(prefix + "sh_dy", listener);
    apvts.addParameterListener(prefix + "eh_dx", listener);
    apvts.addParameterListener(prefix + "eh_dy", listener);
    for (int i = 0; i < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto s = juce::String(i);
        apvts.addParameterListener(prefix + "p" + s + "_on", listener);
        apvts.addParameterListener(prefix + "p" + s + "_x", listener);
        apvts.addParameterListener(prefix + "p" + s + "_y", listener);
        apvts.addParameterListener(prefix + "p" + s + "_idx", listener);
        apvts.addParameterListener(prefix + "p" + s + "_idy", listener);
        apvts.addParameterListener(prefix + "p" + s + "_odx", listener);
        apvts.addParameterListener(prefix + "p" + s + "_ody", listener);
    }
}

static void removeCurveListeners(juce::AudioProcessorValueTreeState& apvts,
                                  juce::AudioProcessorValueTreeState::Listener* listener,
                                  const juce::String& prefix)
{
    apvts.removeParameterListener(prefix + "sh_dx", listener);
    apvts.removeParameterListener(prefix + "sh_dy", listener);
    apvts.removeParameterListener(prefix + "eh_dx", listener);
    apvts.removeParameterListener(prefix + "eh_dy", listener);
    for (int i = 0; i < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto s = juce::String(i);
        apvts.removeParameterListener(prefix + "p" + s + "_on", listener);
        apvts.removeParameterListener(prefix + "p" + s + "_x", listener);
        apvts.removeParameterListener(prefix + "p" + s + "_y", listener);
        apvts.removeParameterListener(prefix + "p" + s + "_idx", listener);
        apvts.removeParameterListener(prefix + "p" + s + "_idy", listener);
        apvts.removeParameterListener(prefix + "p" + s + "_odx", listener);
        apvts.removeParameterListener(prefix + "p" + s + "_ody", listener);
    }
}

WetDiaperAudioProcessor::WetDiaperAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    registerCurveListeners(apvts, &curveParamListener_, "rc_");
    registerCurveListeners(apvts, &curveParamListener_, "lc_");
    updateDisplayCurves();
    rebuildLUT();
}

WetDiaperAudioProcessor::~WetDiaperAudioProcessor()
{
    removeCurveListeners(apvts, &curveParamListener_, "rc_");
    removeCurveListeners(apvts, &curveParamListener_, "lc_");
}

static void addCurveParams(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                           const juce::String& prefix, bool automatable)
{
    auto fid = [&](const juce::String& suffix) {
        return juce::ParameterID(prefix + suffix, 1);
    };
    auto fa = juce::AudioParameterFloatAttributes().withAutomatable(automatable);
    auto ba = juce::AudioParameterBoolAttributes().withAutomatable(automatable);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("sh_dx"), prefix + "StartOut DX",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f / 3.0f, fa));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("sh_dy"), prefix + "StartOut DY",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 1.0f / 3.0f, fa));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("eh_dx"), prefix + "EndIn DX",
        juce::NormalisableRange<float>(-1.0f, 0.0f, 0.001f), -1.0f / 3.0f, fa));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("eh_dy"), prefix + "EndIn DY",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), -1.0f / 3.0f, fa));

    for (int i = 0; i < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto s = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterBool>(fid("p" + s + "_on"), prefix + "P" + s + " On", false, ba));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("p" + s + "_x"), prefix + "P" + s + " X",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f, fa));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("p" + s + "_y"), prefix + "P" + s + " Y",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.5f, fa));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("p" + s + "_idx"), prefix + "P" + s + " InDX",
            juce::NormalisableRange<float>(-1.0f, 0.0f, 0.001f), 0.0f, fa));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("p" + s + "_idy"), prefix + "P" + s + " InDY",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f, fa));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("p" + s + "_odx"), prefix + "P" + s + " OutDX",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f, fa));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fid("p" + s + "_ody"), prefix + "P" + s + " OutDY",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f, fa));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout WetDiaperAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 5 main + 39 right curve = 44 automatable. All fit under Ableton's 64-param threshold.
    // Left curve (39 more) would push to 83, so left curve is non-automatable.

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

    addCurveParams(params, "rc_", true);
    addCurveParams(params, "lc_", false);

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
    peakDecay_ = std::pow(0.01f, 1.0f / static_cast<float>(sampleRate * 0.3));
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
        float blockPeak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            blockPeak = std::max(blockPeak, std::abs(in[i]));

        float prev = inputLevelRms.load(std::memory_order_relaxed);
        float blockDecay = std::pow(peakDecay_, static_cast<float>(numSamples));
        inputLevelRms.store(std::max(blockPeak, prev * blockDecay), std::memory_order_relaxed);
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

    if (curveParamsDirty_.exchange(false, std::memory_order_acquire))
        rebuildLUTFromParams();

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

BezierCurve::SlotValues WetDiaperAudioProcessor::readSlotValues(const juce::String& prefix) const
{
    BezierCurve::SlotValues v;
    v.startOutDx = apvts.getRawParameterValue(prefix + "sh_dx")->load();
    v.startOutDy = apvts.getRawParameterValue(prefix + "sh_dy")->load();
    v.endInDx    = apvts.getRawParameterValue(prefix + "eh_dx")->load();
    v.endInDy    = apvts.getRawParameterValue(prefix + "eh_dy")->load();
    for (int i = 0; i < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto s = juce::String(i);
        auto& slot = v.slots[i];
        slot.on   = apvts.getRawParameterValue(prefix + "p" + s + "_on")->load() >= 0.5f;
        slot.x    = apvts.getRawParameterValue(prefix + "p" + s + "_x")->load();
        slot.y    = apvts.getRawParameterValue(prefix + "p" + s + "_y")->load();
        slot.inDx = apvts.getRawParameterValue(prefix + "p" + s + "_idx")->load();
        slot.inDy = apvts.getRawParameterValue(prefix + "p" + s + "_idy")->load();
        slot.outDx = apvts.getRawParameterValue(prefix + "p" + s + "_odx")->load();
        slot.outDy = apvts.getRawParameterValue(prefix + "p" + s + "_ody")->load();
    }
    return v;
}

void WetDiaperAudioProcessor::writeSlotValues(const juce::String& prefix,
                                               const BezierCurve::SlotValues& vals)
{
    auto set = [&](const juce::String& id, float v) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };
    set(prefix + "sh_dx", vals.startOutDx);
    set(prefix + "sh_dy", vals.startOutDy);
    set(prefix + "eh_dx", vals.endInDx);
    set(prefix + "eh_dy", vals.endInDy);
    for (int i = 0; i < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto s = juce::String(i);
        auto& slot = vals.slots[i];
        set(prefix + "p" + s + "_on", slot.on ? 1.0f : 0.0f);
        set(prefix + "p" + s + "_x", slot.x);
        set(prefix + "p" + s + "_y", slot.y);
        set(prefix + "p" + s + "_idx", slot.inDx);
        set(prefix + "p" + s + "_idy", slot.inDy);
        set(prefix + "p" + s + "_odx", slot.outDx);
        set(prefix + "p" + s + "_ody", slot.outDy);
    }
}

int WetDiaperAudioProcessor::findFreeSlot(const juce::String& prefix) const
{
    for (int i = 0; i < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto s = juce::String(i);
        if (apvts.getRawParameterValue(prefix + "p" + s + "_on")->load() < 0.5f)
            return i;
    }
    return -1;
}

void WetDiaperAudioProcessor::updateDisplayCurves()
{
    auto rcVals = readSlotValues("rc_");
    BezierCurve::buildFromSlots(bezierCurve_, rcVals);
    auto lcVals = readSlotValues("lc_");
    BezierCurve::buildFromSlots(leftBezierCurve_, lcVals);
}

void WetDiaperAudioProcessor::rebuildLUTFromParams()
{
    BezierCurve tempRC, tempLC;
    auto rcVals = readSlotValues("rc_");
    BezierCurve::buildFromSlots(tempRC, rcVals);
    auto lcVals = readSlotValues("lc_");
    BezierCurve::buildFromSlots(tempLC, lcVals);

    int current = activeLutIndex_.load(std::memory_order_acquire);
    int writeIdx = 1 - current;
    tempRC.generateLUT(lutBuffers_[writeIdx]);
    tempLC.generateLUT(leftLutBuffers_[writeIdx]);
    activeLutIndex_.store(writeIdx, std::memory_order_release);
    curveVersion_.fetch_add(1, std::memory_order_relaxed);
}

void WetDiaperAudioProcessor::rebuildLUT()
{
    int current = activeLutIndex_.load(std::memory_order_acquire);
    int writeIdx = 1 - current;
    bezierCurve_.generateLUT(lutBuffers_[writeIdx]);
    leftBezierCurve_.generateLUT(leftLutBuffers_[writeIdx]);
    activeLutIndex_.store(writeIdx, std::memory_order_release);
    curveVersion_.fetch_add(1, std::memory_order_relaxed);
}

static BezierCurve::SlotValues legacyTreeToSlots(const juce::ValueTree& child)
{
    BezierCurve::SlotValues v;

    auto startH = child.getChildWithName("StartHandle");
    if (startH.isValid())
    {
        v.startOutDx = static_cast<float>(startH.getProperty("outDx", 1.0f / 3.0f));
        v.startOutDy = static_cast<float>(startH.getProperty("outDy", 1.0f / 3.0f));
    }

    auto endH = child.getChildWithName("EndHandle");
    if (endH.isValid())
    {
        v.endInDx = static_cast<float>(endH.getProperty("inDx", -1.0f / 3.0f));
        v.endInDy = static_cast<float>(endH.getProperty("inDy", -1.0f / 3.0f));
    }

    int slotIdx = 0;
    for (int i = 0; i < child.getNumChildren() && slotIdx < BezierCurve::SlotValues::kMaxSlots; ++i)
    {
        auto ch = child.getChild(i);
        if (ch.hasType("Point"))
        {
            auto& slot = v.slots[slotIdx];
            slot.on = true;
            slot.x = static_cast<float>(ch.getProperty("x", 0.5f));
            slot.y = static_cast<float>(ch.getProperty("y", 0.5f));
            slot.inDx = static_cast<float>(ch.getProperty("inDx", 0.0f));
            slot.inDy = static_cast<float>(ch.getProperty("inDy", 0.0f));
            slot.outDx = static_cast<float>(ch.getProperty("outDx", 0.0f));
            slot.outDy = static_cast<float>(ch.getProperty("outDy", 0.0f));
            ++slotIdx;
        }
    }

    return v;
}

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
    {
        auto tree = juce::ValueTree::fromXml(*xml);

        BezierCurve::SlotValues legacyRC, legacyLC;
        bool hasLegacyRC = false, hasLegacyLC = false;

        auto bezierChild = tree.getChildWithName("BezierCurve");
        if (bezierChild.isValid())
        {
            legacyRC = legacyTreeToSlots(bezierChild);
            hasLegacyRC = true;
            tree.removeChild(bezierChild, nullptr);
        }

        auto leftChild = tree.getChildWithName("LeftBezierCurve");
        if (leftChild.isValid())
        {
            legacyLC = legacyTreeToSlots(leftChild);
            hasLegacyLC = true;
            tree.removeChild(leftChild, nullptr);
        }

        apvts.replaceState(tree);

        if (hasLegacyRC) writeSlotValues("rc_", legacyRC);
        if (hasLegacyLC) writeSlotValues("lc_", legacyLC);

        updateDisplayCurves();
        rebuildLUT();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WetDiaperAudioProcessor();
}
