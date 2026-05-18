#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "LayoutSizes.h"

namespace KnobDesign
{
    inline const juce::Colour bgColour         { 0xff111111 };
    inline const juce::Colour accentColour     { 0xffd48300 };
    inline const juce::Colour accentHoverColour{ 0xffffb84d };

    inline constexpr float knobStrokeFrac    = 0.033f;
    inline constexpr float indicatorWidthFrac= 0.040f;
    inline constexpr float tickStrokeFrac    = 0.033f;

    inline constexpr float indicatorStart    = 0.33f;
    inline constexpr float indicatorEnd      = 0.75f;

    inline constexpr float tickGap           = 1.15f;
    inline constexpr float tickLength        = 0.18f;

    inline constexpr float rotationStartAngle = -135.0f;
    inline constexpr float rotationEndAngle   =  135.0f;

    inline constexpr float labelFontScale    = 0.18f;
    inline constexpr float gainLabelScale    = 0.06f;
    inline constexpr float dbTextScale       = 0.06f;

    inline constexpr int   defaultWidth      = 650;
    inline constexpr int   defaultHeight     = 370;

    inline constexpr int   minWidth          = 400;
    inline constexpr int   minHeight         = 230;
    inline constexpr int   maxWidth          = 1000;
    inline constexpr int   maxHeight         = 570;

    inline float normToAngleRad(float norm01)
    {
        float degrees = rotationStartAngle + norm01 * (rotationEndAngle - rotationStartAngle);
        return juce::degreesToRadians(degrees);
    }

    inline float stringWidth(const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText(font, text, 0.0f, 0.0f);
        return ga.getBoundingBox(0, -1, true).getWidth();
    }
}

enum class KnobType
{
    Drive,
    Tone
};

class ConjusKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ConjusKnobLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, KnobDesign::accentColour);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, KnobDesign::accentColour);
    }

    void loadFonts(const void* boldData, int boldSize, const void* regularData, int regularSize)
    {
        boldTypeface = juce::Typeface::createSystemTypefaceFor(boldData, static_cast<size_t>(boldSize));
        regularTypeface = juce::Typeface::createSystemTypefaceFor(regularData, static_cast<size_t>(regularSize));
    }

    juce::Font getBoldFont(float height) const
    {
        if (boldTypeface != nullptr)
            return juce::Font(juce::FontOptions(boldTypeface).withHeight(height));
        return juce::Font(juce::FontOptions().withHeight(height).withStyle("Bold"));
    }

    juce::Font getRegularFont(float height) const
    {
        if (regularTypeface != nullptr)
            return juce::Font(juce::FontOptions(regularTypeface).withHeight(height));
        return juce::Font(juce::FontOptions().withHeight(height));
    }

    void drawCornerResizer(juce::Graphics& g, int w, int h,
                           bool isMouseOver, bool isMouseDragging) override
    {
        const auto colour = (isMouseOver || isMouseDragging)
                            ? KnobDesign::accentHoverColour
                            : KnobDesign::accentColour.brighter(0.2f);
        g.setColour(colour);

        const float minDim = juce::jmin(static_cast<float>(w), static_cast<float>(h));
        const float inset = minDim * 0.20f;
        const float right = static_cast<float>(w);
        const float bottom = static_cast<float>(h);
        const float strokeW = juce::jmax(2.5f, minDim * 0.11f);

        for (int i = 1; i <= 3; ++i)
        {
            const float t = static_cast<float>(i) * (bottom - inset) / 4.0f;
            juce::Line<float> line{ right - 2.0f, bottom - t - 2.0f,
                                    right - t - 2.0f, bottom - 2.0f };
            g.drawLine(line, strokeW);
        }
    }

    static void setKnobType(juce::Slider& slider, KnobType type)
    {
        slider.getProperties().set("knobType", static_cast<int>(type));
    }

    static KnobType getKnobType(juce::Slider& slider)
    {
        return static_cast<KnobType>(static_cast<int>(slider.getProperties().getWithDefault("knobType", 0)));
    }

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float /*rotaryStartAngle*/, float /*rotaryEndAngle*/,
                          juce::Slider& slider) override
    {
        using namespace KnobDesign;

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        float sliderW = bounds.getWidth();
        float sliderH = bounds.getHeight();
        float diameter = juce::jmin(juce::jmin(sliderW, sliderH) * 0.78f,
                                    sliderW * 0.60f);
        float radius = diameter * 0.5f;

        float parentH = 0.0f;
        if (auto* editor = slider.getParentComponent())
            parentH = static_cast<float>(editor->getHeight());
        const float knobShiftDown = 90.0f * (parentH > 0.0f
                                             ? parentH / static_cast<float>(KnobDesign::defaultHeight)
                                             : 1.0f);
        float cx = bounds.getCentreX();
        auto* parent = slider.getParentComponent();
        const float windowH = parent ? static_cast<float>(parent->getHeight()) : bounds.getHeight();
        const float sliderY = static_cast<float>(slider.getY());
        float cy = windowH * 0.5f - sliderY + knobShiftDown;

        float strokeW = diameter * knobStrokeFrac;
        float indW = diameter * indicatorWidthFrac;
        float tickW = diameter * tickStrokeFrac;

        float hoverProgress = static_cast<float>(
            slider.getProperties().getWithDefault("hoverProgress", 0.0));
        auto interactiveColour = accentColour.interpolatedWith(accentHoverColour, hoverProgress);

        g.setColour(interactiveColour);
        g.drawEllipse(cx - radius + strokeW * 0.5f,
                      cy - radius + strokeW * 0.5f,
                      diameter - strokeW,
                      diameter - strokeW,
                      strokeW);

        float angle = normToAngleRad(sliderPosProportional);
        float innerR = radius * indicatorStart;
        float outerR = radius * indicatorEnd;

        juce::Path indicator;
        indicator.startNewSubPath(cx + std::sin(angle) * innerR,
                                 cy - std::cos(angle) * innerR);
        indicator.lineTo(cx + std::sin(angle) * outerR,
                         cy - std::cos(angle) * outerR);
        g.strokePath(indicator,
                     juce::PathStrokeType(indW,
                                          juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        float tickStartR = radius * tickGap;
        float tickEndR = radius * (tickGap + tickLength);

        auto knobType = getKnobType(slider);

        float defaultNorm = (knobType == KnobType::Drive) ? 0.31f : 0.5f;

        float tickAngles[3] = {
            juce::degreesToRadians(rotationStartAngle),
            normToAngleRad(defaultNorm),
            juce::degreesToRadians(rotationEndAngle)
        };

        g.setColour(accentColour);
        for (int i = 0; i < 3; ++i)
        {
            juce::Path tick;
            tick.startNewSubPath(cx + std::sin(tickAngles[i]) * tickStartR,
                                cy - std::cos(tickAngles[i]) * tickStartR);
            tick.lineTo(cx + std::sin(tickAngles[i]) * tickEndR,
                        cy - std::cos(tickAngles[i]) * tickEndR);
            g.strokePath(tick,
                         juce::PathStrokeType(tickW,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
        }

        float fontSize = diameter * labelFontScale;
        float markerFontSize = fontSize * 0.85f;
        g.setColour(accentColour);
        g.setFont(getBoldFont(markerFontSize));

        float labelR = tickEndR + markerFontSize * 0.8f;
        float labelYOffset = fontSize * 0.05f;

        juce::String leftLabel, midLabel, rightLabel;
        if (knobType == KnobType::Drive)
        {
            leftLabel  = "1";
            midLabel   = "5";
            rightLabel = "100";
        }
        else
        {
            leftLabel  = "0";
            midLabel   = "50";
            rightLabel = "100";
        }

        float a0 = juce::degreesToRadians(rotationStartAngle);
        float lx0 = cx + std::sin(a0) * labelR;
        float ly0 = cy - std::cos(a0) * labelR + labelYOffset;
        g.drawText(leftLabel,
                   juce::Rectangle<float>(lx0 - fontSize * 2.5f, ly0 - markerFontSize * 0.5f,
                                          fontSize * 5.0f, markerFontSize * 1.2f),
                   juce::Justification::centred, false);

        float a1 = juce::degreesToRadians(rotationEndAngle);
        float lx1 = cx + std::sin(a1) * labelR;
        float ly1 = cy - std::cos(a1) * labelR + labelYOffset;
        g.drawText(rightLabel,
                   juce::Rectangle<float>(lx1 - fontSize * 2.5f, ly1 - markerFontSize * 0.5f,
                                          fontSize * 5.0f, markerFontSize * 1.2f),
                   juce::Justification::centred, false);

        float aMid = normToAngleRad(defaultNorm);
        float topLabelR = tickEndR + markerFontSize * 0.3f;
        float lxM = cx + std::sin(aMid) * topLabelR;
        float lyM = cy - std::cos(aMid) * topLabelR - markerFontSize * 0.5f;
        g.drawText(midLabel,
                   juce::Rectangle<float>(lxM - fontSize * 2.5f, lyM - markerFontSize * 0.5f,
                                          fontSize * 5.0f, markerFontSize * 1.2f),
                   juce::Justification::centred, false);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        bool isSliderTextBox = (dynamic_cast<juce::Slider*>(label.getParentComponent()) != nullptr);

        if (isSliderTextBox)
        {
            auto text = label.getText();
            auto* slider = dynamic_cast<juce::Slider*>(label.getParentComponent());
            auto knobType = slider ? getKnobType(*slider) : KnobType::Drive;

            juce::String suffix = (knobType == KnobType::Tone) ? " %" : "";

            auto* editor = slider ? slider->getParentComponent() : nullptr;
            float windowH = editor ? static_cast<float>(editor->getHeight()) : 370.0f;
            float pillFontSize = windowH * 0.055f;
            auto pillFont = getBoldFont(pillFontSize);

            juce::String valueStr = text.replace(" %", "").trim();

            float valueW = KnobDesign::stringWidth(pillFont, valueStr);
            float suffixW = suffix.isEmpty() ? 0.0f : KnobDesign::stringWidth(pillFont, suffix);
            float totalW = valueW + suffixW;

            float pillH = pillFontSize * 1.5f;
            float padX = pillH * 0.45f;
            float pillW = totalW + 2.0f * padX;

            float labelW = static_cast<float>(label.getWidth());
            float pillX = (labelW - pillW) * 0.5f;
            float pillUpShift = pillFontSize * 1.25f;
            float pillY = (static_cast<float>(label.getHeight()) - pillH) * 0.5f - pillUpShift;

            auto pillBounds = juce::Rectangle<float>(pillX, pillY, pillW, pillH);

            float pillHoverProgress = 0.0f;
            if (auto* parentSlider = label.findParentComponentOfClass<juce::Slider>())
                pillHoverProgress = static_cast<float>(
                    parentSlider->getProperties().getWithDefault("hoverProgress", 0.0));
            auto pillFillColour = KnobDesign::accentColour
                .interpolatedWith(KnobDesign::accentHoverColour, pillHoverProgress);

            g.setColour(pillFillColour);
            g.fillRoundedRectangle(pillBounds, pillH * 0.5f);

            g.setColour(KnobDesign::bgColour);
            g.setFont(pillFont);
            float centreY = pillBounds.getCentreY();

            juce::String displayText = valueStr + suffix;
            g.drawText(displayText,
                       juce::Rectangle<float>(pillBounds.getX(), centreY - pillFontSize * 0.5f,
                                              pillBounds.getWidth(), pillFontSize),
                       juce::Justification::centred, false);
        }
        else
        {
            LookAndFeel_V4::drawLabel(g, label);
        }
    }

private:
    juce::Typeface::Ptr boldTypeface;
    juce::Typeface::Ptr regularTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConjusKnobLookAndFeel)
};
