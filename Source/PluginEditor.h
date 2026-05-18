#pragma once

#include "PluginProcessor.h"
#include "KnobDesign.h"
#include "LayoutSizes.h"
#include "BypassButton.h"
#include "BinaryData.h"

class AnimatedSlider : public juce::Slider
{
public:
    std::function<void()> onDoubleClick;

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (onDoubleClick)
            onDoubleClick();
    }

    bool hitTest(int x, int y) override
    {
        float sw = static_cast<float>(getWidth());
        float sh = static_cast<float>(getHeight());

        float parentH = sh;
        if (auto* editor = getParentComponent())
            parentH = static_cast<float>(editor->getHeight());
        const float knobShift = 90.0f * (parentH / static_cast<float>(KnobDesign::defaultHeight));

        float d = juce::jmin(juce::jmin(sw, sh) * 0.78f, sw * 0.60f);
        float r = d * 0.5f;
        float cx = sw * 0.5f;
        float cy = parentH * 0.5f - static_cast<float>(getY()) + knobShift;

        float dx = static_cast<float>(x) - cx;
        float dy = static_cast<float>(y) - cy;
        if (dx * dx + dy * dy <= r * r) return true;

        float textBoxH = sh * 0.25f;
        int pillHalfW = static_cast<int>(sw * 0.30f);
        int pillTop    = static_cast<int>(sh - textBoxH * 0.85f);
        int pillBottom = static_cast<int>(sh - textBoxH * 0.05f);
        return (x >= static_cast<int>(cx) - pillHalfW
             && x <= static_cast<int>(cx) + pillHalfW
             && y >= pillTop
             && y <= pillBottom);
    }
};

class WetDiaperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit WetDiaperAudioProcessorEditor(WetDiaperAudioProcessor&);
    ~WetDiaperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void setChromeVisible(bool visible);

private:
    bool showChrome = true;
    void timerCallback() override;

    WetDiaperAudioProcessor& processorRef;
    ConjusKnobLookAndFeel conjusLAF;

    AnimatedSlider driveSlider;
    AnimatedSlider toneSlider;
    juce::Label driveLabel { {}, "DRIVE" };
    juce::Label toneLabel  { {}, "TONE" };

    BypassButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;

    juce::Image logoImage;

    std::unique_ptr<juce::ResizableCornerComponent> resizer;

    juce::Rectangle<int> logoBounds;
    bool  logoHoverTarget   = false;
    float logoHoverProgress = 0.0f;

    struct SliderAnimation
    {
        bool active = false;
        double targetValue = 0.0;
        double currentValue = 0.0;
    };
    SliderAnimation driveAnim;
    SliderAnimation toneAnim;

    void startSnapAnimation(juce::Slider& slider, SliderAnimation& anim);
    void updateSnapAnimation(juce::Slider& slider, SliderAnimation& anim);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WetDiaperAudioProcessorEditor)
};
