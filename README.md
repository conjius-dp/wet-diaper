# wet-diaper

A distortion/overdrive audio plugin built with JUCE 8.0.12.

Available as VST3, AU, and Standalone on macOS and Windows.

## Features

- **Drive** — Controls the amount of distortion (soft-clip tanh waveshaping)
- **Tone** — Post-distortion low-pass filter (dark to bright)
- **Bypass** — Host-integrated bypass button
- Zero latency

## Build

Requires CMake 3.22+, Ninja, and a C++17 compiler.

```bash
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Test

```bash
cmake --build build --target WetDiaperTests
./build/WetDiaperTests
```
