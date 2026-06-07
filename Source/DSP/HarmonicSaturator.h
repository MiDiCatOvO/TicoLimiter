#pragma once
#include <cmath>
#include <algorithm>

namespace dsp {

// Harmonic saturator with independent odd/even harmonic control
// Odd harmonics (3rd, 5th): warm, tube-like character
// Even harmonics (2nd, 4th): bright, rich character
class HarmonicSaturator {
public:
    HarmonicSaturator() = default;

    void prepare(double /*sampleRate*/) {}

    // drive: 0.0 (clean) to 1.0 (heavy saturation)
    void setDrive(float drive) {
        mDrive = std::clamp(drive, 0.0f, 1.0f);
    }

    // mix: 0.0 = all even harmonics, 1.0 = all odd harmonics
    void setOddEvenMix(float mix) {
        mOddMix = std::clamp(mix, 0.0f, 1.0f);
        mEvenMix = 1.0f - mOddMix;
    }

    // Process one sample
    float process(float input) {
        if (mDrive < 0.001f) return input;

        float x = input;
        float driveAmount = mDrive * 2.0f; // scale up for audible effect

        // Odd harmonics: soft-symmetric clipping (x - x^3/3 style)
        // Using polynomial: x + a3*x^3 + a5*x^5
        float odd = x;
        if (driveAmount > 0.0f) {
            float x2 = x * x;
            float x3 = x2 * x;
            float x5 = x3 * x2;
            // Coefficients scaled to keep output reasonable
            float a3 = driveAmount * 0.15f;
            float a5 = driveAmount * 0.05f;
            odd = x + a3 * x3 + a5 * x5;
            // Soft clamp to prevent runaway
            odd = std::tanh(odd);
        }

        // Even harmonics: asymmetric saturation (adds even-order terms)
        // Using: x + a2*x^2 + a4*x^4
        float even = x;
        if (driveAmount > 0.0f) {
            float x2 = x * x;
            float x4 = x2 * x2;
            float a2 = driveAmount * 0.2f;
            float a4 = driveAmount * 0.08f;
            even = x + a2 * x2 * (x > 0.0f ? 1.0f : -1.0f) + a4 * x4;
            even = std::tanh(even);
        }

        // Mix odd and even
        float saturated = mOddMix * odd + mEvenMix * even;

        // Dry/wet mix based on drive
        return x + mDrive * (saturated - x);
    }

    // Process a stereo pair
    void processStereo(float& left, float& right) {
        left = process(left);
        right = process(right);
    }

private:
    float mDrive = 0.0f;
    float mOddMix = 0.5f;
    float mEvenMix = 0.5f;
};

} // namespace dsp
