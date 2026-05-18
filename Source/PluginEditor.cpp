#include "PluginEditor.h"

WetDiaperAudioProcessorEditor::WetDiaperAudioProcessorEditor(WetDiaperAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    conjusLAF.loadFonts(BinaryData::InconsolataBold_ttf,
                        BinaryData::InconsolataBold_ttfSize,
                        BinaryData::InconsolataRegular_ttf,
                        BinaryData::InconsolataRegular_ttfSize);
    setLookAndFeel(&conjusLAF);

    driveSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 120, 30);
    ConjusKnobLookAndFeel::setKnobType(driveSlider, KnobType::Drive);
    driveSlider.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible(driveSlider);

    toneSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    toneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 120, 30);
    ConjusKnobLookAndFeel::setKnobType(toneSlider, KnobType::Tone);
    toneSlider.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible(toneSlider);

    driveLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(driveLabel);
    toneLabel.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(toneLabel);

    driveLabel.toBack();
    toneLabel.toBack();

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "drive", driveSlider);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "tone", toneSlider);

    driveSlider.textFromValueFunction = [](double value) -> juce::String {
        return juce::String(value, 1);
    };
    driveSlider.valueFromTextFunction = [](const juce::String& text) -> double {
        return text.getDoubleValue();
    };
    driveSlider.updateText();

    toneSlider.textFromValueFunction = [](double value) -> juce::String {
        return juce::String(value, 1) + " %";
    };
    toneSlider.valueFromTextFunction = [](const juce::String& text) -> double {
        return text.getDoubleValue();
    };
    toneSlider.updateText();

    driveSlider.onDoubleClick = [this]() {
        startSnapAnimation(driveSlider, driveAnim);
    };
    toneSlider.onDoubleClick = [this]() {
        startSnapAnimation(toneSlider, toneAnim);
    };

    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "bypass", bypassButton);

    logoImage = juce::ImageCache::getFromMemory(
        BinaryData::conjiusavatartransparentbg_png,
        BinaryData::conjiusavatartransparentbg_pngSize);

    int savedW = processorRef.editorWidth.load();
    int savedH = processorRef.editorHeight.load();
    setResizable(true, false);
    setResizeLimits(KnobDesign::minWidth, KnobDesign::minHeight,
                    KnobDesign::maxWidth, KnobDesign::maxHeight);
    resizer = std::make_unique<juce::ResizableCornerComponent>(this, getConstrainer());
    resizer->setLookAndFeel(&conjusLAF);
    addAndMakeVisible(resizer.get());
    getConstrainer()->setFixedAspectRatio(
        static_cast<double>(KnobDesign::defaultWidth) / KnobDesign::defaultHeight);
    setSize(savedW, savedH);

    addMouseListener(this, true);
    startTimerHz(60);
}

WetDiaperAudioProcessorEditor::~WetDiaperAudioProcessorEditor()
{
    if (resizer) resizer->setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    stopTimer();
}

void WetDiaperAudioProcessorEditor::setChromeVisible(bool visible)
{
    showChrome = visible;
    bypassButton.setVisible(visible);
    repaint();
}

void WetDiaperAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    auto pos = getLocalPoint(e.eventComponent, e.getPosition());
    bool inside = logoBounds.contains(pos);
    if (inside != logoHoverTarget)
        logoHoverTarget = inside;
}

void WetDiaperAudioProcessorEditor::mouseExit(const juce::MouseEvent& e)
{
    if (e.eventComponent == this)
        logoHoverTarget = false;
}

void WetDiaperAudioProcessorEditor::timerCallback()
{
    float target = logoHoverTarget ? 1.0f : 0.0f;
    if (std::abs(target - logoHoverProgress) > 0.002f)
    {
        logoHoverProgress += (target - logoHoverProgress) * 0.18f;
        repaint(logoBounds.expanded(static_cast<int>(logoBounds.getWidth() * 0.2f)));
    }

    auto animateHover = [](juce::Component& c, bool hovered) {
        float current = static_cast<float>(c.getProperties().getWithDefault("hoverProgress", 0.0));
        float dest = hovered ? 1.0f : 0.0f;
        if (std::abs(dest - current) > 0.002f)
        {
            current += (dest - current) * 0.22f;
            c.getProperties().set("hoverProgress", current);
            c.repaint();
        }
    };
    animateHover(driveSlider, driveSlider.isMouseOverOrDragging(true));
    animateHover(toneSlider, toneSlider.isMouseOverOrDragging(true));

    updateSnapAnimation(driveSlider, driveAnim);
    updateSnapAnimation(toneSlider, toneAnim);
}

void WetDiaperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(KnobDesign::bgColour);

    if (logoImage.isValid() && showChrome)
    {
        float scale = static_cast<float>(getWidth()) / static_cast<float>(KnobDesign::defaultWidth);
        int baseSize = static_cast<int>(37.5f * scale);
        int padLeft = static_cast<int>(6.0f * scale);
        int baseX = padLeft;
        int baseY = getHeight() - baseSize;
        logoBounds = { baseX, baseY, baseSize, baseSize };

        float hoverScale = 1.0f + 0.2f * logoHoverProgress;
        int drawSize = static_cast<int>(baseSize * hoverScale);
        int drawX = baseX + (baseSize - drawSize) / 2;
        int drawY = baseY + (baseSize - drawSize) / 2;

        float brightness = 0.35f + 0.65f * logoHoverProgress;
        g.setOpacity(brightness);
        g.drawImage(logoImage,
                    drawX, drawY, drawSize, drawSize,
                    0, 0, logoImage.getWidth(), logoImage.getHeight());
        g.setOpacity(1.0f);
    }

    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());

    {
        float titleFontSize = h * 0.065f;
        auto titleFont = conjusLAF.getBoldFont(titleFontSize);
        g.setFont(titleFont);
        g.setColour(KnobDesign::accentColour);
        float titleY = h * 0.12f;
        g.drawText("WET DIAPER",
                   juce::Rectangle<float>(0.0f, titleY, w, titleFontSize * 1.2f),
                   juce::Justification::centred, false);

        float subFontSize = titleFontSize * 0.45f;
        auto subFont = conjusLAF.getBoldFont(subFontSize);
        g.setFont(subFont);
        g.setColour(KnobDesign::accentHoverColour);
        g.drawText("DISTORTION",
                   juce::Rectangle<float>(0.0f, titleY + titleFontSize * 1.1f, w, subFontSize * 1.4f),
                   juce::Justification::centred, false);
    }
}

void WetDiaperAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    const float scaleF  = static_cast<float>(getWidth())
                        / static_cast<float>(KnobDesign::defaultWidth);
    const float pad     = 30.0f * scaleF;
    const float borderW = 4.0f  * scaleF;
    const float radius  = 70.0f * scaleF;
    juce::Rectangle<float> borderRect{ pad, pad,
                                       static_cast<float>(getWidth())  - 2.0f * pad,
                                       static_cast<float>(getHeight()) - 2.0f * pad };
    juce::Path border;
    border.addRoundedRectangle(borderRect, radius);
    g.setColour(KnobDesign::accentColour);
    g.strokePath(border, juce::PathStrokeType(borderW));
}

void WetDiaperAudioProcessorEditor::resized()
{
    processorRef.editorWidth.store(getWidth());
    processorRef.editorHeight.store(getHeight());

    if (resizer != nullptr)
    {
        const int handleSize = 28;
        resizer->setBounds(getWidth() - handleSize, getHeight() - handleSize,
                           handleSize, handleSize);
        resizer->toFront(false);
        resizer->repaint();
    }

    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());

    {
        const float scaleF  = w / static_cast<float>(KnobDesign::defaultWidth);
        const float edgeGap = 10.0f * scaleF;
        const float btnSize = 34.0f * scaleF;
        const float btnX = static_cast<float>(getWidth()) - edgeGap - btnSize;
        const float btnY = edgeGap;
        bypassButton.setBounds(static_cast<int>(btnX),
                               static_cast<int>(btnY),
                               static_cast<int>(btnSize),
                               static_cast<int>(btnSize));
        bypassButton.toFront(false);
    }

    float margin = w * 0.05f;
    float knobColW = w * 0.40f;
    float knobColX0 = margin;
    float knobColX1 = w - margin - knobColW;

    const float labelFontSize = KnobDesign::columnLabelFontSize(w);
    auto labelFont = conjusLAF.getBoldFont(labelFontSize);
    driveLabel.setFont(labelFont);
    toneLabel.setFont(labelFont);

    const int labelH = static_cast<int>(KnobDesign::columnLabelHeight(w));
    const int labelY = static_cast<int>(h * 0.25f);
    driveLabel.setBounds(static_cast<int>(knobColX0), labelY,
                         static_cast<int>(knobColW), labelH);
    toneLabel.setBounds(static_cast<int>(knobColX1), labelY,
                        static_cast<int>(knobColW), labelH);

    float dbFontSize = w * KnobDesign::dbTextScale;
    int sliderBottom = static_cast<int>(h * 0.96f);
    int sliderTop = static_cast<int>(h * 0.04f);
    int sliderH = sliderBottom - sliderTop;

    const float knobClusterExtraShift = 20.0f * (h / static_cast<float>(KnobDesign::defaultHeight));
    const int sliderTopEditor = sliderTop + static_cast<int>(knobClusterExtraShift);

    float sliderBoundsW = knobColW * 0.90f;
    float sliderOffset0 = knobColX0 + (knobColW - sliderBoundsW) * 0.5f;
    float sliderOffset1 = knobColX1 + (knobColW - sliderBoundsW) * 0.5f;

    int textBoxW = static_cast<int>(sliderBoundsW * 0.95f);
    int textBoxH = static_cast<int>(dbFontSize * 2.6f);

    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxW, textBoxH);
    driveSlider.setMouseDragSensitivity(static_cast<int>(w * 0.5f));
    driveSlider.setBounds(static_cast<int>(sliderOffset0), sliderTopEditor,
                          static_cast<int>(sliderBoundsW), sliderH);

    toneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxW, textBoxH);
    toneSlider.setMouseDragSensitivity(static_cast<int>(w * 0.5f));
    toneSlider.setBounds(static_cast<int>(sliderOffset1), sliderTopEditor,
                         static_cast<int>(sliderBoundsW), sliderH);

    for (auto* slider : { &driveSlider, &toneSlider })
    {
        slider->setPaintingIsUnclipped(true);
        if (auto* textBox = slider->getChildComponent(0))
        {
            if (auto* label = dynamic_cast<juce::Label*>(textBox))
            {
                label->setFont(conjusLAF.getRegularFont(dbFontSize));
                label->setMinimumHorizontalScale(1.0f);
                label->setColour(juce::Label::textColourId, KnobDesign::accentColour);
                label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
                label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
                label->setInterceptsMouseClicks(false, false);
                label->setPaintingIsUnclipped(true);
            }
        }
    }

    float knobAreaH = static_cast<float>(sliderH) - static_cast<float>(textBoxH);
    float knobDiameter = juce::jmin(sliderBoundsW, knobAreaH) * 0.78f;
    float knobStrokeW = knobDiameter * KnobDesign::knobStrokeFrac;
    bypassButton.setRingStrokeWidth(knobStrokeW);
}

void WetDiaperAudioProcessorEditor::startSnapAnimation(juce::Slider& slider, SliderAnimation& anim)
{
    auto* param = processorRef.getAPVTS().getParameter(
        &slider == &driveSlider ? "drive" : "tone");
    if (param == nullptr) return;

    anim.currentValue = slider.getValue();
    anim.targetValue = static_cast<double>(param->getDefaultValue())
                       * (slider.getMaximum() - slider.getMinimum()) + slider.getMinimum();
    anim.active = true;
}

void WetDiaperAudioProcessorEditor::updateSnapAnimation(juce::Slider& slider, SliderAnimation& anim)
{
    if (!anim.active) return;

    constexpr double smoothing = 0.15;
    anim.currentValue += (anim.targetValue - anim.currentValue) * smoothing;

    if (std::abs(anim.targetValue - anim.currentValue) < 0.01)
    {
        anim.currentValue = anim.targetValue;
        anim.active = false;
    }

    slider.setValue(anim.currentValue, juce::sendNotificationAsync);
}
