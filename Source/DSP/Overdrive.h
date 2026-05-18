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

    float getDrive() const { return drive_; }
    float getTone() const { return tone_; }
    double getSampleRate() const { return sampleRate_; }

private:
    double sampleRate_ = 44100.0;
    float drive_ = 5.0f;
    float tone_ = 50.0f;

    // One-pole low-pass for tone control
    float toneStateL_ = 0.0f;
    float toneCoeff_ = 0.0f;

    void updateToneCoeff();

    // Map tone (0-100) to cutoff frequency (200 Hz - 20 kHz, logarithmic)
    static float toneToFreq(float tone);

    // Soft-clip waveshaping: normalised tanh
    static float waveshape(float sample, float drive);
};
