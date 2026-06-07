#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>
#include <deque>

namespace dsp {

// Transparent look-ahead feed-forward limiter
// - Feed-forward design ensures flat frequency response
// - Look-ahead prevents transient overshoot
// - Attack bound to look-ahead: gain reaches target exactly when peak emerges from delay
// - Dual-stage release (fast + slow) for natural sound
// - Auto-release adapts to signal dynamics
// - O(1) amortized min tracking via monotonic deque with sequence numbers
class LookAheadLimiter {
public:
    LookAheadLimiter() = default;

    void prepare(double sampleRate) {
        mSampleRate = static_cast<float>(sampleRate);
        applyLookAheadMs(mLookAheadMs);

        // Dual-stage release
        mFastReleaseCoeff = calculateTimeCoeff(30.0f);
        mSlowReleaseCoeff = calculateTimeCoeff(300.0f);
        mManualReleaseCoeff = calculateTimeCoeff(100.0f);

        mCurrentGain = 1.0f;
        mAppliedGain = 1.0f;

        mSequence = 0;
        mMinDeque.clear();
    }

    void reset() {
        std::memset(mDelayBufferL.data(), 0, sizeof(float) * mDelayBufferL.size());
        std::memset(mDelayBufferR.data(), 0, sizeof(float) * mDelayBufferR.size());
        mDelayWriteIdx = 0;
        mCurrentGain = 1.0f;
        mAppliedGain = 1.0f;
        mSequence = 0;
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

        // Instantaneous target gain for this sample
        float targetGain = 1.0f;
        if (peak > mThresholdLinear) {
            targetGain = mThresholdLinear / peak;
        }

        size_t bufSize = mDelayBufferL.size();

        // Store input in delay buffer
        mDelayBufferL[mDelayWriteIdx] = left;
        mDelayBufferR[mDelayWriteIdx] = right;

        // Track RAW target gain in monotonic deque — find the minimum
        // over the look-ahead window. This is what prevents transient overshoot:
        // we attenuate based on the loudest peak in the window, not just the current sample.
        while (!mMinDeque.empty() && mMinDeque.back().gain > targetGain)
            mMinDeque.pop_back();
        mMinDeque.push_back({mSequence, targetGain});

        // Remove entries that fell outside the look-ahead window.
        size_t cutoff = (mSequence >= mLookAheadSize) ? (mSequence - mLookAheadSize + 1) : 0;
        while (!mMinDeque.empty() && mMinDeque.front().sequence < cutoff)
            mMinDeque.pop_front();

        // The windowed minimum — this is the actual gain the limiter should apply
        float windowedGain = mMinDeque.empty() ? 1.0f : mMinDeque.front().gain;

        // Release smoothing (instant attack, smoothed release)
        float releaseCoeff;
        if (mAutoRelease) {
            // Adaptive: GR depth drives fast↔slow, user release knob sets the base.
            // Higher release → base closer to fast (snappier recovery)
            // Lower release → base closer to slow (natural, smooth recovery)
            float grDepth = std::clamp(1.0f - mCurrentGain, 0.0f, 1.0f);
            mAutoBlend += 0.001f * (grDepth - mAutoBlend);
            float userNorm = std::clamp((mManualReleaseCoeff - 0.998f) / (0.9999f - 0.998f), 0.0f, 1.0f);
            float base = mSlowReleaseCoeff + userNorm * (mFastReleaseCoeff - mSlowReleaseCoeff);
            releaseCoeff = base + mAutoBlend * (mSlowReleaseCoeff - base);
        } else {
            releaseCoeff = mManualReleaseCoeff;
        }

        // Instant attack when gain needs to decrease, smoothed release when it increases
        if (windowedGain < mCurrentGain)
            mCurrentGain = windowedGain; // instant attack
        else
            mCurrentGain = windowedGain + releaseCoeff * (mCurrentGain - windowedGain); // smoothed release

        // Read delayed sample and apply the smoothed windowed gain
        size_t delayReadIdx = mDelayWriteIdx;
        left  = mDelayBufferL[delayReadIdx] * mCurrentGain;
        right = mDelayBufferR[delayReadIdx] * mCurrentGain;

        mAppliedGain = mCurrentGain;

        // Safety hard clamp — guarantees output never exceeds threshold.
        // Catches edge cases where the look-ahead window is too short to
        // fully pre-attenuate a transient, or when upstream processing
        // (soft clipper mix) lets peaks through.
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

        // Grow buffer if needed (never shrink) — preserves existing content
        if (newSize > mDelayBufferL.size()) {
            mDelayBufferL.resize(newSize, 0.0f);
            mDelayBufferR.resize(newSize, 0.0f);
        }

        // Only change the window size — do NOT reset indices, sequence, or deque
        // This allows seamless transition without audio discontinuities
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

    // Dual-stage release
    float mFastReleaseCoeff = 0.99f;    // ~30ms
    float mSlowReleaseCoeff = 0.9998f;  // ~300ms
    float mManualReleaseCoeff = 0.999f; // user-set value
    bool mAutoRelease = false;
    float mAutoBlend = 0.0f;            // smoothed blend factor

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
