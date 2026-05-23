# wet-diaper

<p align="center">
  <picture><img src="https://conjius-dp.github.io/wet-diaper/screenshot.png" width="373" alt="Wet Diaper"></picture>
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/wet-diaper/releases/latest/download/WetDiaper-macOS-VST3.zip"><img src="Assets/badges/download-vst3-macos.png" height="32" alt="Download VST3 for macOS"></a>
  &nbsp;
  <a href="https://github.com/conjius-dp/wet-diaper/releases/latest/download/WetDiaper-macOS-AU.zip"><img src="Assets/badges/download-au-macos.png" height="32" alt="Download AU for macOS"></a>
  &nbsp;
  <a href="https://github.com/conjius-dp/wet-diaper/releases/latest/download/WetDiaper-Windows-VST3.zip"><img src="Assets/badges/download-vst3-windows.png" height="32" alt="Download VST3 for Windows"></a>
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/wet-diaper/actions/workflows/ci.yml"><img src="https://github.com/conjius-dp/wet-diaper/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/conjius-dp/wet-diaper/actions/workflows/ci.yml"><img src="https://img.shields.io/endpoint?url=https%3A%2F%2Fconjius-dp.github.io%2Fwet-diaper%2Fcoverage.json" alt="Coverage"></a>
  <a href="https://github.com/conjius-dp/wet-diaper/releases/latest"><img src="https://img.shields.io/github/v/release/conjius-dp/wet-diaper?label=stable&color=brightgreen" alt="Stable"></a>
  <a href="https://github.com/conjius-dp/wet-diaper/releases"><img src="https://img.shields.io/github/v/release/conjius-dp/wet-diaper?include_prereleases&label=nightly" alt="Nightly"></a>
  <a href="https://github.com/conjius-dp/wet-diaper/releases"><img src="https://img.shields.io/github/downloads/conjius-dp/wet-diaper/total?label=downloads&color=blue" alt="Total downloads"></a>
</p>

Distortion/overdrive effect. Soft-clip tanh waveshaping with adjustable tone filter. Drive from clean identity to hard clip.

VST3 (macOS, Windows), AU + Standalone (macOS). Stereo in/out. Zero latency.

## Usage

1. Set **Drive** for distortion amount (0 = clean, 100 = hard clip).
2. Adjust **Tone** to shape the post-distortion frequency response.
3. Power button (top-right) hard-bypasses the plugin.

## Parameters

| Parameter | Range | Default |
|---|---|---|
| Drive | 0 - 100 | 5.0 |
| Tone | 0% - 100% | 50.0% |
| Bypass | on / off | off |

## Build

```bash
git clone https://github.com/conjius-dp/wet-diaper.git
cd wet-diaper
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache
cmake --build build
```

VST3 / AU bundles are auto-copied to `~/Library/Audio/Plug-Ins/`.

Requires JUCE 8.0.12, CMake >= 3.22, a C++17 compiler.

## Tests

```bash
cmake --build build --target WetDiaperTests && ./build/WetDiaperTests
```

## macOS: plugin won't open after download

macOS quarantines anything downloaded from a browser and blocks unsigned plugins with a Gatekeeper dialog. Strip the quarantine flag once after dropping the plugin into its folder:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/wet-diaper.vst3
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/wet-diaper.component
```

Restart your DAW and the plugin loads silently.
