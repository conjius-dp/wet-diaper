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
    drive_ = std::clamp(driveAmount, 0.0f, 100.0f);
}

void Overdrive::setTone(float toneNorm)
{
    tone_ = std::clamp(toneNorm, 0.0f, 100.0f);
    updateToneCoeff();
}

void Overdrive::setVolume(float v)
{
    volume_ = std::clamp(v, 0.0f, 1.0f);
}

void Overdrive::setLUT(const float* rightData, const float* leftData, int size)
{
    rightLut_ = rightData;
    leftLut_ = leftData;
    lutSize_ = size;
}

void Overdrive::processMono(float* samples, int numSamples)
{
    float gain = (drive_ < 1e-6f) ? 1.0f : std::pow(100.0f, drive_ / 100.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        float wet;
        if (rightLut_ != nullptr && leftLut_ != nullptr && lutSize_ > 1)
            wet = lookupLUT(samples[i], gain, rightLut_, leftLut_, lutSize_);
        else
            wet = waveshape(samples[i], drive_);

        toneStateL_ += toneCoeff_ * (wet - toneStateL_);
        wet = toneStateL_;

        samples[i] = wet * volume_;
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
    if (drive < 1e-6f)
        return sample;
    return std::tanh(sample * drive) / std::tanh(drive);
}

float Overdrive::lookupLUT(float sample, float gain, const float* rightLut, const float* leftLut, int lutSize)
{
    float x = sample * gain;
    bool negative = x < 0.0f;
    if (negative) x = -x;
    if (x > 1.0f) x = 1.0f;

    const float* lut = negative ? leftLut : rightLut;
    float pos = x * static_cast<float>(lutSize - 1);
    int idx = static_cast<int>(pos);
    float frac = pos - static_cast<float>(idx);
    if (idx >= lutSize - 1)
        return negative ? -lut[lutSize - 1] : lut[lutSize - 1];

    float result = lut[idx] + frac * (lut[idx + 1] - lut[idx]);
    return negative ? -result : result;
}
