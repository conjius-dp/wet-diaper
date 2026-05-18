#include "DSP/Overdrive.h"
#include <algorithm>
#include <cmath>

void Overdrive::prepare(double sampleRate, int /*maxBlockSize*/)
{
    sampleRate_ = sampleRate;
    toneStateL_ = 0.0f;
    updateToneCoeff();
}

void Overdrive::reset()
{
    toneStateL_ = 0.0f;
}

void Overdrive::setDrive(float driveAmount)
{
    drive_ = std::clamp(driveAmount, 1.0f, 100.0f);
}

void Overdrive::setTone(float toneNorm)
{
    tone_ = std::clamp(toneNorm, 0.0f, 100.0f);
    updateToneCoeff();
}

void Overdrive::processMono(float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float wet = waveshape(samples[i], drive_);

        toneStateL_ += toneCoeff_ * (wet - toneStateL_);
        wet = toneStateL_;

        samples[i] = wet;
    }
}

float Overdrive::toneToFreq(float tone)
{
    const float minFreq = 200.0f;
    const float maxFreq = 20000.0f;
    float norm = tone / 100.0f;
    return minFreq * std::pow(maxFreq / minFreq, norm);
}

void Overdrive::updateToneCoeff()
{
    if (sampleRate_ <= 0.0) return;
    float freq = toneToFreq(tone_);
    float dt = 1.0f / static_cast<float>(sampleRate_);
    float rc = 1.0f / (2.0f * 3.14159265f * freq);
    toneCoeff_ = dt / (rc + dt);
}

float Overdrive::waveshape(float sample, float drive)
{
    constexpr float preGain = 30.0f;
    float x = sample * preGain * drive;
    return std::tanh(x);
}
