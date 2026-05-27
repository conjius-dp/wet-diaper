#pragma once

#include <cmath>

class Overdrive
{
public:
    Overdrive() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void processMono(float* samples, int numSamples);

    void setDrive(float driveAmount);
    void setTone(float toneNorm);
    void setVolume(float v);
    void setLUT(const float* rightData, const float* leftData, int size);

    float getDrive() const { return drive_; }
    float getTone() const { return tone_; }
    float getVolume() const { return volume_; }
    double getSampleRate() const { return sampleRate_; }

private:
    double sampleRate_ = 44100.0;
    float drive_ = 5.0f;
    float tone_ = 50.0f;
    float volume_ = 1.0f;

    float toneStateL_ = 0.0f;
    float toneCoeff_ = 0.0f;

    const float* rightLut_ = nullptr;
    const float* leftLut_ = nullptr;
    int lutSize_ = 0;

    void updateToneCoeff();
    static float toneToFreq(float tone);
    static float waveshape(float sample, float drive);
    static float lookupLUT(float sample, float gain, const float* rightLut, const float* leftLut, int lutSize);
};
