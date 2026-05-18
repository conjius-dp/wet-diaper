#pragma once

namespace KnobDesign
{
    inline constexpr float knobClusterShiftDefault = 20.0f;

    inline constexpr int   defaultWindowWidth  = 650;
    inline constexpr int   defaultWindowHeight = 370;

    inline constexpr float columnLabelFontSize(float w) { return 0.033f * w; }

    inline constexpr float columnLabelHeight(float w)
    {
        return columnLabelFontSize(w) * 1.2f;
    }
}
