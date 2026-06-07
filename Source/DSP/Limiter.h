#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>
#include <deque>

namespace dsp {

// L2-style look-ahead feed-forward limiter
// - Instant attack via look-ahead window
// - Ultra-fast adaptive release with hold for maximum loudness
// - Dual-speed release: fast (5ms) for transients, slow (50ms) for sustained
// - Release speed adapts to GR depth and user setting
// - O(1) amortized min tracking via monotonic deque
class LookAheadLimiter {
public:
    LookAheadLimiter() = default;

    void prepare(double sampleRate) {
        mSampleRate = static_cast<float>(sampleRate);
        applyLookAheadMs(mLookAheadMs);

        // L2-style release: much faster than before
        mFastReleaseCoeff = calculateTimeCoeff(5.0f);   // 5ms — snappy
        mSlowReleaseCoeff = calculateTimeCoeff(50.0f);   // 50ms — still fast
        mManualReleaseCoeff = calculateTimeCoeff(100.0f);

        mCurrentGain = 1.0f;
        mAppliedGain = 1.0f;

        mSequence = 0;
        mHoldCounter = 0;
        mMinDeque.clear();
    }

    void reset() {
        std::memset(mDelayBufferL.data(), 0, sizeof(float) * mDelayBufferL.size());
        std::memset(mDelayBufferR.data(), 0, sizeof(float) * mDelayBufferR.size());
        mDelayWriteIdx = 0;
        mCurrentGain = 1.0f;
        mAppliedGain = 1.0f;
        mSequence = 0;
        mHoldCounter = 0;
        mMinDeque.clear();
    }

    void setThresholdDb(float thresholdDb) {
        mThresholdLinear = std::pow(10.0f, thresholdDb / 20.0f);
    }

    void setReleaseMs(float releaseMs) {
        mManualReleaseCoeff = calculateTimeCoeff(releaseMs);
    }

    void setAutoRelease(bool enabled) { mAutoRelease = enabled; }
    bool getAutoRelease() const { return mAutoRelease; }

    void setLookAheadMs(float ms) {
        ms = std::clamp(ms, 0.0f, 10.0f);
        if (std::abs(ms - mLookAheadMs) < 0.01f) return;
        mLookAheadMs = ms;
        applyLookAheadMs(ms);
    }

    float getLatencySamples() const {
        return static_cast<float>(mLookAheadSize > 1 ? mLookAheadSize - 1 : 0);
    }

    float getLookAheadMs() const { return mLookAheadMs; }

    float getGainReductionDb() const {
        if (mAppliedGain >= 1.0f) return 0.0f;
        return 20.0f * std::log10(std::max(mAppliedGain, 0.0001f));
    }

    void processSample(float& left, float& right) {
        float peak = std::max(std::abs(left), std::abs(right));

        // Instantaneous target gain
        float targetGain = 1.0f;
        if (peak > mThresholdLinear) {
            targetGain = mThresholdLinear / peak;
        }

        size_t bufSize = mDelayBufferL.size();

        // Store input in delay buffer
        mDelayBufferL[mDelayWriteIdx] = left;
        mDelayBufferR[mDelayWriteIdx] = right;

        // Track raw target gain in monotonic deque
        while (!mMinDeque.empty() && mMinDeque.back().gain > targetGain)
            mMinDeque.pop_back();
        mMinDeque.push_back({mSequence, targetGain});

        size_t cutoff = (mSequence >= mLookAheadSize) ? (mSequence - mLookAheadSize + 1) : 0;
        while (!mMinDeque.empty() && mMinDeque.front().sequence < cutoff)
            mMinDeque.pop_front();

        float windowedGain = mMinDeque.empty() ? 1.0f : mMinDeque.front().gain;

        // === L2-style gain control ===

        // Instant attack: snap to the windowed minimum immediately
        if (windowedGain < mCurrentGain) {
            mCurrentGain = windowedGain;
            // Reset hold counter — keep gain low for a few samples
            // This prevents the gain from bouncing back too quickly on transients
            mHoldCounter = mHoldSamples;
        } else {
            // Release phase
            if (mHoldCounter > 0) {
                // Hold: keep gain at minimum briefly before releasing
                mHoldCounter--;
                // Don't change mCurrentGain during hold
            } else {
                // Adaptive release: blend fast and slow based on GR depth + user setting
                float releaseCoeff;

                if (mAutoRelease) {
                    // GR depth: deeper GR → use slow release to avoid pumping
                    float grDepth = std::clamp(1.0f - mCurrentGain, 0.0f, 1.0f);

                    // Smooth the GR depth to avoid jitter
                    mAutoBlend += 0.005f * (grDepth - mAutoBlend);

                    // User release knob maps to the fast release speed
                    // Higher release value → faster release → more aggressive loudness
                    float userNorm = std::clamp((mManualReleaseCoeff - 0.998f) / (0.9999f - 0.998f), 0.0f, 1.0f);

                    // Fast component: driven by user setting (higher = faster)
                    float fastCoeff = mSlowReleaseCoeff + userNorm * (mFastReleaseCoeff - mSlowReleaseCoeff);

                    // Blend: deep GR → slow release, shallow GR → fast release
                    releaseCoeff = fastCoeff + mAutoBlend * (mSlowReleaseCoeff - fastCoeff);
                } else {
                    releaseCoeff = mManualReleaseCoeff;
                }

                mCurrentGain = windowedGain + releaseCoeff * (mCurrentGain - windowedGain);
            }
        }

        // Read delayed sample and apply gain
        size_t delayReadIdx = mDelayWriteIdx;
        left  = mDelayBufferL[delayReadIdx] * mCurrentGain;
        right = mDelayBufferR[delayReadIdx] * mCurrentGain;

        mAppliedGain = mCurrentGain;

        // Safety hard clamp
        {
            float outPeak = std::max(std::abs(left), std::abs(right));
            if (outPeak > mThresholdLinear) {
                float clampGain = mThresholdLinear / outPeak;
                left  *= clampGain;
                right *= clampGain;
                if (clampGain < mCurrentGain)
                    mAppliedGain = clampGain;
            }
        }

        // Advance write pointer
        mDelayWriteIdx = (mDelayWriteIdx + 1) % bufSize;
        mSequence++;
    }

    void processBlock(float* left, float* right, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            processSample(left[i], right[i]);
        }
    }

private:
    void applyLookAheadMs(float ms) {
        size_t newSize;
        if (ms <= 0.0f) {
            newSize = 2;
        } else {
            auto lookAheadSamples = static_cast<size_t>(mSampleRate * ms * 0.001f);
            lookAheadSamples = std::max(lookAheadSamples, size_t(1));
            newSize = lookAheadSamples + 1;
        }

        if (newSize == mLookAheadSize && !mDelayBufferL.empty()) {
            return;
        }

        if (newSize > mDelayBufferL.size()) {
            mDelayBufferL.resize(newSize, 0.0f);
            mDelayBufferR.resize(newSize, 0.0f);
        }

        mLookAheadSize = newSize;
    }

    float calculateTimeCoeff(float timeMs) {
        float samples = mSampleRate * timeMs * 0.001f;
        return std::exp(-1.0f / std::max(samples, 1.0f));
    }

    struct DequeEntry {
        size_t sequence;
        float gain;
    };

    float mSampleRate = 44100.0f;
    float mLookAheadMs = 5.0f;
    float mThresholdLinear = 1.0f;

    // L2-style fast release
    float mFastReleaseCoeff = 0.99f;    // ~5ms
    float mSlowReleaseCoeff = 0.999f;   // ~50ms
    float mManualReleaseCoeff = 0.999f; // user-set value
    bool mAutoRelease = false;
    float mAutoBlend = 0.0f;

    // Gain hold — prevents premature gain recovery on transients
    static constexpr int mHoldSamples = 8; // ~0.17ms at 48kHz
    int mHoldCounter = 0;

    float mCurrentGain = 1.0f;
    float mAppliedGain = 1.0f;

    size_t mLookAheadSize = 221;
    size_t mDelayWriteIdx = 0;
    size_t mSequence = 0;

    std::vector<float> mDelayBufferL;
    std::vector<float> mDelayBufferR;

    std::deque<DequeEntry> mMinDeque;
};

} // namespace dsp
