#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>

namespace dsp {

// Proper half-band FIR filter for oversampling
// Uses windowed-sinc method with Kaiser window
// 31 taps per phase, >90dB stopband attenuation
class HalfBandFilter {
public:
    static constexpr int kNumTaps = 63;
    static constexpr int kHalfLen = (kNumTaps - 1) / 2; // 31

    HalfBandFilter() {
        // Design half-band lowpass filter using windowed sinc
        // Cutoff at pi/2 (Nyquist/2)
        constexpr float beta = 9.0f; // Kaiser beta for ~90dB attenuation
        for (int i = 0; i <= kHalfLen; ++i) {
            float n = static_cast<float>(i - kHalfLen);
            float coeff;
            if (std::abs(n) < 0.0001f) {
                coeff = 0.5f; // sinc(0.5) = 2/pi * pi/2 / 1 = but for half-band: 0.5
            } else {
                // sinc(pi/2 * n) * (1/2)
                coeff = std::sin(static_cast<float>(M_PI) * 0.5f * n) /
                        (static_cast<float>(M_PI) * n);
            }
            // Kaiser window
            float r = n / static_cast<float>(kHalfLen);
            float w = kaiserWindow(r, beta);
            mCoeffs[i] = coeff * w * 2.0f; // scale for unity gain
        }
        reset();
    }

    void reset() {
        std::fill(mDelayLine.begin(), mDelayLine.end(), 0.0f);
        mWritePos = 0;
    }

    // Process one sample
    float process(float input) {
        mDelayLine[mWritePos] = input;
        float sum = 0.0f;
        int idx = mWritePos;
        for (int i = 0; i < kNumTaps; ++i) {
            sum += mDelayLine[idx] * getCoeff(i);
            idx = (idx + 1) % kNumTaps;
        }
        mWritePos = (mWritePos + 1) % kNumTaps;
        return sum;
    }

private:
    float getCoeff(int i) const {
        // Symmetric coefficients
        if (i <= kHalfLen)
            return mCoeffs[i];
        return mCoeffs[kNumTaps - 1 - i];
    }

    static float kaiserWindow(float r, float beta) {
        if (r < -1.0f || r > 1.0f) return 0.0f;
        float x = r * r;
        // Approximate I0 using polynomial
        float i0x = 1.0f + x * (0.25f + x * (0.015625f + x * 0.000434f));
        float i0b = 1.0f + beta * beta * (0.25f + beta * beta * (0.015625f + beta * beta * 0.000434f));
        return i0x / i0b;
    }

    float mCoeffs[kHalfLen + 1] = {};
    std::array<float, kNumTaps> mDelayLine = {};
    int mWritePos = 0;
};

// 2x oversampler that processes entire blocks
class TwoXOverSamplerBlock {
public:
    TwoXOverSamplerBlock() = default;

    void prepare(int maxBlockSize) {
        mUpBuffer.resize(maxBlockSize * 2, 0.0f);
        mDownBuffer.resize(maxBlockSize * 2, 0.0f);
        reset();
    }

    void reset() {
        mUpFilterL.reset();
        mUpFilterR.reset();
        mDownFilterL.reset();
        mDownFilterR.reset();
    }

    // Upsample stereo block: numSamplesIn -> numSamplesIn * 2
    void upsampleBlock(const float* inL, const float* inR,
                       float* outL, float* outR, int numSamplesIn) {
        int outIdx = 0;
        for (int i = 0; i < numSamplesIn; ++i) {
            // Insert original sample
            outL[outIdx] = mUpFilterL.process(inL[i]);
            outR[outIdx] = mUpFilterR.process(inR[i]);
            outIdx++;
            // Insert zero (polyphase)
            outL[outIdx] = mUpFilterL.process(0.0f);
            outR[outIdx] = mUpFilterR.process(0.0f);
            outIdx++;
        }
    }

    // Downsample stereo block: numSamplesIn -> numSamplesIn / 2
    // numSamplesIn must be even
    void downsampleBlock(const float* inL, const float* inR,
                         float* outL, float* outR, int numSamplesIn) {
        int outIdx = 0;
        for (int i = 0; i < numSamplesIn; i += 2) {
            mDownFilterL.process(inL[i]);
            outL[outIdx] = mDownFilterL.process(inL[i + 1]);
            mDownFilterR.process(inR[i]);
            outR[outIdx] = mDownFilterR.process(inR[i + 1]);
            outIdx++;
        }
    }

private:
    HalfBandFilter mUpFilterL, mUpFilterR;
    HalfBandFilter mDownFilterL, mDownFilterR;
    std::vector<float> mUpBuffer, mDownBuffer;
};

// Multi-stage oversampling engine (block-based)
template<int NumStages = 4>
class OverSamplingEngine {
public:
    static constexpr int kFactor = 1 << NumStages; // 2^NumStages

    OverSamplingEngine() = default;

    void prepare(int maxBlockSize) {
        int size = maxBlockSize;
        for (int s = 0; s < NumStages; ++s) {
            size *= 2;
            mStageBufferL[s].resize(size, 0.0f);
            mStageBufferR[s].resize(size, 0.0f);
            mStages[s].prepare(maxBlockSize << s);
        }
    }

    void reset() {
        for (auto& stage : mStages)
            stage.reset();
    }

    // Upsample stereo block by kFactor
    // Returns pointer to oversampled buffers and the oversampled size
    int upsample(const float* inL, const float* inR, int numSamples,
                 const float*& outL, const float*& outR) {
        // Stage 0: 1x -> 2x
        mStages[0].upsampleBlock(inL, inR,
            mStageBufferL[0].data(), mStageBufferR[0].data(), numSamples);

        // Remaining stages: each doubles the previous
        int currentSize = numSamples * 2;
        for (int s = 1; s < NumStages; ++s) {
            mStages[s].upsampleBlock(
                mStageBufferL[s-1].data(), mStageBufferR[s-1].data(),
                mStageBufferL[s].data(), mStageBufferR[s].data(),
                currentSize);
            currentSize *= 2;
        }

        outL = mStageBufferL[NumStages - 1].data();
        outR = mStageBufferR[NumStages - 1].data();
        return currentSize; // numSamples * kFactor
    }

    // Downsample stereo block by kFactor
    void downsample(const float* inL, const float* inR, int oversampledSize,
                    float* outL, float* outR) {
        // Work backwards through stages
        int currentSize = oversampledSize;

        // Last stage downsample
        const float* srcL = inL;
        const float* srcR = inR;

        for (int s = NumStages - 1; s >= 1; --s) {
            mStages[s].downsampleBlock(srcL, srcR,
                mStageBufferL[s-1].data(), mStageBufferR[s-1].data(),
                currentSize);
            srcL = mStageBufferL[s-1].data();
            srcR = mStageBufferR[s-1].data();
            currentSize /= 2;
        }

        // First stage downsample to output
        mStages[0].downsampleBlock(srcL, srcR, outL, outR, currentSize);
    }

private:
    std::array<TwoXOverSamplerBlock, NumStages> mStages;
    std::array<std::vector<float>, NumStages> mStageBufferL;
    std::array<std::vector<float>, NumStages> mStageBufferR;
};

} // namespace dsp
