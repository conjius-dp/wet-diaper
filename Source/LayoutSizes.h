#pragma once

// Pure-arithmetic sizing helpers for the editor layout.
// No spectrum graph in this plugin — the full window is the knob area.

namespace KnobDesign
{
    inline constexpr float knobClusterShiftDefault = 20.0f;

    inline constexpr int   defaultWindowWidth  = 650;
    inline constexpr int   defaultWindowHeight = 370;

    // Column-label text (DRIVE / TONE)
    inline constexpr float columnLabelFontSize(float w) { return 0.033f * w; }

    inline constexpr float columnLabelHeight(float w)
    {
        return columnLabelFontSize(w) * 1.2f;
    }
}
