#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/Overdrive.h"
#include "DSP/BezierCurve.h"
#include "KnobDesign.h"

class WetDiaperAudioProcessor : public juce::AudioProcessor
{
public:
    WetDiaperAudioProcessor();
    ~WetDiaperAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    juce::AudioParameterBool* getBypassParameter() const override
    {
        return dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("bypass"));
    }

    float getAlgorithmicLatencyMs() const
    {
        const double sr = drives[0].getSampleRate();
        if (sr <= 0.0) return 0.0f;
        return static_cast<float>(static_cast<double>(getLatencySamples()) / sr * 1000.0);
    }

    std::atomic<int> editorWidth  { KnobDesign::defaultWidth };
    std::atomic<int> editorHeight { KnobDesign::defaultHeight };
    std::atomic<float> inputLevelRms { 0.0f };

    const BezierCurve& getBezierCurve() const { return bezierCurve_; }
    const BezierCurve& getLeftBezierCurve() const { return leftBezierCurve_; }
    void rebuildLUT();
    bool isSymmetric() const { return apvts.getRawParameterValue("asymmetric")->load() < 0.5f; }
    std::atomic<int> curveVersion_ { 0 };

    BezierCurve::SlotValues readSlotValues(const juce::String& prefix) const;
    void writeSlotValues(const juce::String& prefix, const BezierCurve::SlotValues& vals);
    int findFreeSlot(const juce::String& prefix) const;
    void updateDisplayCurves();

private:
    juce::AudioProcessorValueTreeState apvts;
    Overdrive drives[2];
    BezierCurve bezierCurve_;
    BezierCurve leftBezierCurve_;
    float lutBuffers_[2][BezierCurve::kLutSize]{};
    float leftLutBuffers_[2][BezierCurve::kLutSize]{};
    std::atomic<int> activeLutIndex_ { 0 };
    std::atomic<bool> curveParamsDirty_ { false };

    float peakDecay_ = 0.9999f;

    void rebuildLUTFromParams();

    struct CurveParamListener : juce::AudioProcessorValueTreeState::Listener
    {
        std::atomic<bool>& dirty;
        explicit CurveParamListener(std::atomic<bool>& d) : dirty(d) {}
        void parameterChanged(const juce::String&, float) override { dirty.store(true, std::memory_order_release); }
    };
    CurveParamListener curveParamListener_ { curveParamsDirty_ };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WetDiaperAudioProcessor)
};
