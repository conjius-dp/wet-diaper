// Headless CLI that renders the plugin editor to a single PNG.
// Usage: WetDiaperScreenshot <output.png> [width] [height] [scaleFactor]

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <random>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "KnobDesign.h"

static int parseInt(const char* s, int fallback)
{
    if (s == nullptr) return fallback;
    const auto str = juce::String(s).trim();
    return str.isEmpty() ? fallback : str.getIntValue();
}

static float parseFloat(const char* s, float fallback)
{
    if (s == nullptr) return fallback;
    const auto str = juce::String(s).trim();
    return str.isEmpty() ? fallback : static_cast<float>(str.getDoubleValue());
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <output.png> [width] [height] [scale]\n";
        return 1;
    }

    const juce::File outFile{ juce::String::fromUTF8(argv[1]) };
    const int   width  = parseInt (argc > 2 ? argv[2] : nullptr, KnobDesign::defaultWidth);
    const int   height = parseInt (argc > 3 ? argv[3] : nullptr, KnobDesign::defaultHeight);
    const float scale  = parseFloat(argc > 4 ? argv[4] : nullptr, 2.0f);

    juce::ScopedJuceInitialiser_GUI scopedGUI;

    WetDiaperAudioProcessor processor;
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    processor.prepareToPlay(sampleRate, blockSize);

    // Feed some audio through so the plugin is in a valid state
    {
        juce::AudioBuffer<float> buf(2, blockSize);
        juce::MidiBuffer midi;
        for (int i = 0; i < 10; ++i)
        {
            buf.clear();
            // Generate a sine wave
            for (int n = 0; n < blockSize; ++n)
            {
                float s = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(n) / static_cast<float>(sampleRate));
                buf.setSample(0, n, s);
                buf.setSample(1, n, s);
            }
            processor.processBlock(buf, midi);
        }
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor{ processor.createEditor() };
    if (editor == nullptr)
    {
        std::cerr << "error: createEditor() returned null\n";
        return 2;
    }
    editor->setSize(width, height);

    if (auto* wdEditor = dynamic_cast<WetDiaperAudioProcessorEditor*>(editor.get()))
        wdEditor->setChromeVisible(false);

    auto snap = editor->createComponentSnapshot(editor->getLocalBounds(), false, scale);

    // Crop and mask corners
    {
        const float widthScale = static_cast<float>(width) / static_cast<float>(KnobDesign::defaultWidth);
        const int insetPx = static_cast<int>(20.0f * widthScale * scale);
        const float cornerRadius = 70.0f * widthScale * scale;

        const int outW = snap.getWidth() - 2 * insetPx;
        const int outH = snap.getHeight() - 2 * insetPx;

        juce::Image framed{ juce::Image::ARGB, outW, outH, true };
        juce::Graphics rg{ framed };

        juce::Path mask;
        mask.addRoundedRectangle(0.0f, 0.0f,
                                 static_cast<float>(outW),
                                 static_cast<float>(outH),
                                 cornerRadius);
        rg.reduceClipRegion(mask);
        rg.drawImageAt(snap, -insetPx, -insetPx);

        snap = framed;
    }

    outFile.deleteFile();
    juce::FileOutputStream stream{ outFile };
    if (!stream.openedOk())
    {
        std::cerr << "error: cannot write to " << outFile.getFullPathName() << "\n";
        return 3;
    }

    juce::PNGImageFormat png;
    if (!png.writeImageToStream(snap, stream))
    {
        std::cerr << "error: PNG encode failed\n";
        return 4;
    }

    std::cout << "wrote " << outFile.getFullPathName()
              << " (" << snap.getWidth() << "x" << snap.getHeight() << ")\n";
    return 0;
}
