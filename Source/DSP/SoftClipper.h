#pragma once
#include <cmath>
#include <algorithm>

namespace dsp {

// Soft clipper using tanh saturation curve
// y = tanh(x * drive) / tanh(drive)
// Smooth clipping with no hard knee, preserves transients musically
class SoftClipper {
public:
    SoftClipper() = default;

    void prepare(double /*sampleRate*/) {
        mDrive = 1.0f;
        mInvTanhDrive = 1.0f;
    }

    // Set drive amount (1.0 = no clipping, higher = more clipping)
    // thresholdDb: the level at which clipping begins (in dB relative to 0dBFS)
    void setThresholdDb(float thresholdDb) {
        // Convert threshold to linear: if threshold is -3dB, input at 0dBFS
        // gets clipped. We set drive so tanh(drive * threshold_linear) / tanh(drive)
        // starts compressing at the threshold level.
        float thresholdLinear = std::pow(10.0f, thresholdDb / 20.0f);
        // At threshold, we want the transfer function to start curving
        // Higher drive = more aggressive clipping
        mDrive = 2.0f / std::max(thresholdLinear, 0.001f);
        mDrive = std::clamp(mDrive, 1.0f, 50.0f);
        mInvTanhDrive = 1.0f / std::tanh(mDrive);
    }

    void setDrive(float drive) {
        // drive: 0.0 = no effect, 1.0 = heavy clipping
        mDrive = 1.0f + drive * 49.0f; // range: 1.0 to 50.0
        mInvTanhDrive = 1.0f / std::tanh(mDrive);
    }

    // Process one sample through soft clipper
    float process(float input) {
        float output = std::tanh(input * mDrive) * mInvTanhDrive;
        // Track gain reduction
        float inputAbs = std::abs(input);
        if (inputAbs > 0.0001f) {
            float gr = std::abs(output) / inputAbs;
            mGainReduction = mGainReduction * 0.95f + gr * 0.05f;
        }
        return output;
    }

    float getGainReductionDb() const {
        if (mGainReduction >= 1.0f) return 0.0f;
        return 20.0f * std::log10(std::max(mGainReduction, 0.0001f));
    }

    // Process a buffer in-place
    void processBlock(float* buffer, int numSamples) {
        float d = mDrive;
        float inv = mInvTanhDrive;
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = std::tanh(buffer[i] * d) * inv;
        }
    }

private:
    float mDrive = 1.0f;
    float mInvTanhDrive = 1.0f;
    float mGainReduction = 1.0f;
};

} // namespace dsp
