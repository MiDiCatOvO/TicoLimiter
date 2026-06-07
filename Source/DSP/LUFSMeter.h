#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <atomic>

class LUFSMeter {
public:
    LUFSMeter() {
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 5; ++i) {
                mPreFilter[ch].coeffs[i] = 0.0;
                mRLB[ch].coeffs[i] = 0.0;
            }
            mPreFilter[ch].z1 = 0.0;
            mPreFilter[ch].z2 = 0.0;
            mRLB[ch].z1 = 0.0;
            mRLB[ch].z2 = 0.0;
        }
    }

    void prepare(double sampleRate) {
        mSampleRate = sampleRate;

        const int momBlocks = static_cast<int>(0.4 * sampleRate) / kBlockSize;
        const int shtBlocks = static_cast<int>(3.0 * sampleRate) / kBlockSize;
        mBlocksPerSec = sampleRate / kBlockSize;

        mMomBuffer.assign(std::max(momBlocks, 1), 0.0);
        mShtBuffer.assign(std::max(shtBlocks, 1), 0.0);
        mMomIdx.store(0, std::memory_order_relaxed);
        mShtIdx.store(0, std::memory_order_relaxed);
        mMomReady.store(false, std::memory_order_relaxed);
        mShtReady.store(false, std::memory_order_relaxed);

        mUngatedSum = 0.0;
        mUngatedCount = 0;
        mGatedSum = 0.0;
        mGatedCount = 0;
        mLastIntegrated.store(-70.0f, std::memory_order_relaxed);

        computeCoefficients(sampleRate);
        reset();
    }

    void reset() {
        for (int ch = 0; ch < 2; ++ch) {
            mPreFilter[ch].z1 = 0.0;
            mPreFilter[ch].z2 = 0.0;
            mRLB[ch].z1 = 0.0;
            mRLB[ch].z2 = 0.0;
        }
        mSampleCount = 0;
        mBlockSum = 0.0;
    }

    void process(const float* const* channelData, int numChannels, int numSamples) {
        const int chL = 0;
        const int chR = numChannels >= 2 ? 1 : 0;

        // Check if input is silent
        float maxAbs = 0.0f;
        for (int s = 0; s < numSamples; ++s) {
            maxAbs = std::max(maxAbs, std::abs(channelData[chL][s]));
            if (numChannels >= 2)
                maxAbs = std::max(maxAbs, std::abs(channelData[chR][s]));
        }

        // If silent, decay the buffers
        if (maxAbs < 1e-6f) {
            mSilentCount += numSamples;
            if (mSilentCount > static_cast<int>(mSampleRate * 0.1)) {  // 100ms of silence
                // Decay momentary and short-term buffers
                for (auto& v : mMomBuffer) v *= 0.95;
                for (auto& v : mShtBuffer) v *= 0.95;
                mUngatedSum *= 0.95;
                mGatedSum *= 0.95;
            }
            return;
        }

        mSilentCount = 0;

        for (int s = 0; s < numSamples; ++s) {
            double L = static_cast<double>(channelData[chL][s]);
            double R = static_cast<double>(channelData[chR][s]);

            double Lf = processBiquad(mPreFilter[0], L);
            double Rf = processBiquad(mPreFilter[0], R);
            Lf = processBiquad(mRLB[0], Lf);
            Rf = processBiquad(mRLB[0], Rf);

            mBlockSum += Lf * Lf + Rf * Rf;
            ++mSampleCount;

            if (mSampleCount >= kBlockSize) {
                processBlock();
                mSampleCount = 0;
                mBlockSum = 0.0;
            }
        }
    }

    void getLoudness(float& momentary, float& shortTerm, float& integrated) const {
        const int momSize = static_cast<int>(mMomBuffer.size());
        const int shtSize = static_cast<int>(mShtBuffer.size());

        // Snapshot the write indices (audio thread may advance them concurrently)
        int momIdx = mMomIdx.load(std::memory_order_relaxed);
        int shtIdx = mShtIdx.load(std::memory_order_relaxed);

        double momSum = 0.0;
        if (mMomReady.load(std::memory_order_relaxed)) {
            for (int i = 0; i < momSize; ++i)
                momSum += mMomBuffer[i];
            momentary = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(momSum / momSize, 1e-20)));
        } else if (momIdx > 0) {
            for (int i = 0; i < momIdx; ++i)
                momSum += mMomBuffer[i];
            momentary = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(momSum / momIdx, 1e-20)));
        } else {
            momentary = -70.0f;
        }

        double shtSum = 0.0;
        if (mShtReady.load(std::memory_order_relaxed)) {
            for (int i = 0; i < shtSize; ++i)
                shtSum += mShtBuffer[i];
            shortTerm = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(shtSum / shtSize, 1e-20)));
        } else if (shtIdx > 0) {
            for (int i = 0; i < shtIdx; ++i)
                shtSum += mShtBuffer[i];
            shortTerm = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(shtSum / shtIdx, 1e-20)));
        } else {
            shortTerm = -70.0f;
        }

        integrated = mLastIntegrated.load(std::memory_order_relaxed);
    }

private:
    struct BiquadState {
        double coeffs[5]; // b0, b1, b2, a1, a2
        double z1, z2;
    };

    static constexpr int kBlockSize = 480;

    BiquadState mPreFilter[2];
    BiquadState mRLB[2];
    double mSampleRate = 48000.0;

    int mSampleCount = 0;
    double mBlockSum = 0.0;

    std::vector<double> mMomBuffer;
    std::vector<double> mShtBuffer;
    std::atomic<int> mMomIdx{0};
    std::atomic<int> mShtIdx{0};
    std::atomic<bool> mMomReady{false};
    std::atomic<bool> mShtReady{false};
    int mBlocksPerSec = 100;

    double mUngatedSum = 0.0;
    int mUngatedCount = 0;
    double mGatedSum = 0.0;
    int mGatedCount = 0;
    std::atomic<float> mLastIntegrated{-70.0f};
    int mSilentCount = 0;  // Track silence duration

    void computeCoefficients(double fs) {
        auto computeShelf = [&](double gain, double fc, BiquadState& state) {
            double A = std::pow(10.0, gain / 40.0);
            double w0 = 2.0 * M_PI * fc / fs;
            double cosw = std::cos(w0);
            double sinw = std::sin(w0);
            double alpha = sinw / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / 0.7071067811865476 - 1.0) + 2.0);
            double sqA2a = 2.0 * std::sqrt(A) * alpha;

            double b0 = A * ((A + 1) + (A - 1) * cosw + sqA2a);
            double b1 = -2.0 * A * ((A - 1) + (A + 1) * cosw);
            double b2 = A * ((A + 1) + (A - 1) * cosw - sqA2a);
            double a0 = (A + 1) - (A - 1) * cosw + sqA2a;
            double a1 = 2.0 * ((A - 1) - (A + 1) * cosw);
            double a2 = (A + 1) - (A - 1) * cosw - sqA2a;

            state.coeffs[0] = b0 / a0;
            state.coeffs[1] = b1 / a0;
            state.coeffs[2] = b2 / a0;
            state.coeffs[3] = a1 / a0;
            state.coeffs[4] = a2 / a0;
        };

        computeShelf(4.0, 1681.974450955533, mPreFilter[0]);
        computeShelf(0.0, 3813.547087602444, mRLB[0]);
    }

    double processBiquad(BiquadState& s, double x) {
        double y = s.coeffs[0] * x + s.z1;
        s.z1 = s.coeffs[1] * x - s.coeffs[3] * y + s.z2;
        s.z2 = s.coeffs[2] * x - s.coeffs[4] * y;
        return y;
    }

    void processBlock() {
        double ms = mBlockSum / kBlockSize;
        const double absGate = std::pow(10.0, (-70.0 + 0.691) / 10.0);

        // Write block value, then advance index (order matters for reader)
        int mi = mMomIdx.load(std::memory_order_relaxed);
        mMomBuffer[mi] = ms;
        mMomIdx.store((mi + 1) % static_cast<int>(mMomBuffer.size()), std::memory_order_release);
        if (mi + 1 == static_cast<int>(mMomBuffer.size()))
            mMomReady.store(true, std::memory_order_relaxed);

        int si = mShtIdx.load(std::memory_order_relaxed);
        mShtBuffer[si] = ms;
        mShtIdx.store((si + 1) % static_cast<int>(mShtBuffer.size()), std::memory_order_release);
        if (si + 1 == static_cast<int>(mShtBuffer.size()))
            mShtReady.store(true, std::memory_order_relaxed);

        mUngatedSum += ms;
        ++mUngatedCount;

        if (ms > absGate) {
            mGatedSum += ms;
            ++mGatedCount;
        }

        // Compute integrated loudness (relative-gated) on audio thread
        if (mUngatedCount > 0) {
            double ungatedAvg = mUngatedSum / mUngatedCount;
            double relativeGate = ungatedAvg * 0.1; // -10 dB
            double reGatedSum = 0.0;
            int reGatedCount = 0;
            int shtSize = static_cast<int>(mShtBuffer.size());
            int currentShtIdx = si + 1;
            for (int i = 0; i < shtSize; ++i) {
                if (mShtBuffer[i] > relativeGate) {
                    reGatedSum += mShtBuffer[i];
                    ++reGatedCount;
                }
            }
            if (reGatedCount > 0) {
                float val = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(reGatedSum / reGatedCount, 1e-20)));
                mLastIntegrated.store(val, std::memory_order_relaxed);
            }
        }
    }
};
