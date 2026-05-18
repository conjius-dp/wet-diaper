#include <juce_core/juce_core.h>
#include "DSP/Overdrive.h"
#include <cmath>

class OverdriveTests : public juce::UnitTest
{
public:
    OverdriveTests() : juce::UnitTest("Overdrive") {}

    void runTest() override
    {
        beginTest("prepare sets sample rate");
        {
            Overdrive od;
            od.prepare(48000.0, 512);
            expectEquals(od.getSampleRate(), 48000.0);
        }

        beginTest("reset does not crash");
        {
            Overdrive od;
            od.prepare(44100.0, 512);
            od.reset();
        }

        beginTest("silence in, silence out");
        {
            Overdrive od;
            od.prepare(44100.0, 512);
            od.setDrive(5.0f);
            od.setTone(50.0f);

            std::vector<float> buf(512, 0.0f);
            od.processMono(buf.data(), 512);

            for (auto s : buf)
                expectEquals(s, 0.0f);
        }

        beginTest("low drive produces moderate output");
        {
            Overdrive od;
            od.prepare(44100.0, 512);
            od.setDrive(1.0f);
            od.setTone(100.0f);

            // With preGain=30 and drive=1, input 0.5 -> tanh(15) ~= 1.0
            // So even low drive saturates a hot signal
            std::vector<float> warmup(4096, 0.01f);
            od.processMono(warmup.data(), 4096);

            std::vector<float> buf(512, 0.01f);
            od.processMono(buf.data(), 512);

            // tanh(0.01 * 30 * 1) = tanh(0.3) ~= 0.291
            float last = buf.back();
            expect(last > 0.1f && last < 0.5f,
                   "low drive with quiet input should produce moderate output");
        }

        beginTest("high drive clips signal");
        {
            Overdrive od;
            od.prepare(44100.0, 512);
            od.setDrive(100.0f);
            od.setTone(100.0f);

            std::vector<float> warmup(4096, 0.01f);
            od.processMono(warmup.data(), 4096);

            std::vector<float> buf(512, 0.01f);
            od.processMono(buf.data(), 512);

            // tanh(0.01 * 30 * 100) = tanh(30) ~= 1.0
            float last = buf.back();
            expect(last > 0.9f,
                   "high drive should clip to near 1.0");
        }

        beginTest("output is bounded [-1, 1]");
        {
            Overdrive od;
            od.prepare(44100.0, 512);
            od.setDrive(50.0f);
            od.setTone(100.0f);

            std::vector<float> buf(1024);
            for (int i = 0; i < 1024; ++i)
                buf[i] = std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);

            od.processMono(buf.data(), 1024);

            for (auto s : buf)
            {
                expect(s >= -1.01f && s <= 1.01f,
                       "output should be bounded");
            }
        }

        beginTest("tone=0 is dark (attenuates highs)");
        {
            Overdrive od;
            od.prepare(44100.0, 512);
            od.setDrive(5.0f);
            od.setTone(0.0f); // 200 Hz cutoff

            // Feed a 10 kHz sine
            std::vector<float> buf(4096);
            for (int i = 0; i < 4096; ++i)
                buf[i] = 0.5f * std::sin(2.0f * 3.14159f * 10000.0f * i / 44100.0f);

            od.processMono(buf.data(), 4096);

            // Last samples should be heavily attenuated
            float rms = 0.0f;
            for (int i = 3072; i < 4096; ++i)
                rms += buf[i] * buf[i];
            rms = std::sqrt(rms / 1024.0f);

            expect(rms < 0.15f,
                   "tone=0 should heavily attenuate 10 kHz");
        }

        beginTest("drive parameter clamping");
        {
            Overdrive od;
            od.setDrive(-5.0f);
            expectEquals(od.getDrive(), 1.0f);
            od.setDrive(200.0f);
            expectEquals(od.getDrive(), 100.0f);
        }

        beginTest("tone parameter clamping");
        {
            Overdrive od;
            od.setTone(-10.0f);
            expectEquals(od.getTone(), 0.0f);
            od.setTone(150.0f);
            expectEquals(od.getTone(), 100.0f);
        }

        beginTest("no latency");
        {
            // Overdrive is a sample-by-sample processor, no buffering
            Overdrive od;
            od.prepare(44100.0, 512);
            od.setDrive(1.0f);
            od.setTone(100.0f);

            float sample = 0.1f;
            od.processMono(&sample, 1);

            // Should produce non-zero output immediately
            expect(sample != 0.0f, "should process immediately with no latency");
        }
    }
};

static OverdriveTests overdriveTests;

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (auto* result = runner.getResult(i))
            failures += result->failures;

    return failures > 0 ? 1 : 0;
}
