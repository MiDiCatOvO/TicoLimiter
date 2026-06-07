#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace dsp {

// 2nd-order Linkwitz-Riley crossover filter (12dB/oct)
struct LRCrossover {
    float a0, a1, a2, b1, b2;

    struct State {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };

    void prepare(double sampleRate, float freq, bool lowpass) {
        float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
        float cosw = std::cos(w0);
        float sinw = std::sin(w0);
        float alpha = sinw / (2.0f * 0.5f); // Q=0.5 for LR

        if (lowpass) {
            a0 = (1.0f - cosw) * 0.5f;
            a1 = 1.0f - cosw;
            a2 = (1.0f - cosw) * 0.5f;
        } else {
            a0 = (1.0f + cosw) * 0.5f;
            a1 = -(1.0f + cosw);
            a2 = (1.0f + cosw) * 0.5f;
        }
        b1 = -2.0f * cosw;
        b2 = 1.0f - alpha;

        float a0r = 1.0f + alpha;
        a0 /= a0r; a1 /= a0r; a2 /= a0r;
        b1 /= a0r; b2 /= a0r;
    }

    float process(float x, State& s) {
        float y = a0 * x + a1 * s.x1 + a2 * s.x2 - b1 * s.y1 - b2 * s.y2;
        s.x2 = s.x1; s.x1 = x;
        s.y2 = s.y1; s.y1 = y;
        return y;
    }
};

// Per-band compressor
struct BandCompressor {
    float threshold = 1.0f;
    float ratio = 4.0f;
    float attackCoeff = 0.999f;
    float releaseCoeff = 0.9999f;
    float envelope = 0.0f;

    void prepare(double sampleRate) {
        attackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.001f));
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.05f));
        envelope = 0.0f;
    }

    void setThresholdDb(float db) {
        threshold = std::pow(10.0f, db / 20.0f);
    }

    void setRatio(float r) { ratio = r; }

    float process(float x) {
        float inputAbs = std::abs(x);

        if (inputAbs > envelope)
            envelope = attackCoeff * (envelope - inputAbs) + inputAbs;
        else
            envelope = releaseCoeff * (envelope - inputAbs) + inputAbs;

        if (envelope > threshold) {
            float overDb = 20.0f * std::log10(envelope / threshold);
            float gainDb = -overDb * (1.0f - 1.0f / ratio);
            return x * std::pow(10.0f, gainDb / 20.0f);
        }
        return x;
    }
};

// 3-band compressor with Linkwitz-Riley crossovers
class MultiBandCompressor {
public:
    static constexpr int kNumBands = 3;

    void prepare(double sampleRate) {
        mSampleRate = sampleRate;

        // Crossover filters: LP and HP at each split point
        mLowMidLP.prepare(sampleRate, 200.0f, true);
        mLowMidHP.prepare(sampleRate, 200.0f, false);
        mMidHighLP.prepare(sampleRate, 2500.0f, true);
        mMidHighHP.prepare(sampleRate, 2500.0f, false);

        for (auto& bc : mBands)
            bc.prepare(sampleRate);

        reset();
    }

    void reset() {
        for (auto& s : mLowMidLPState)  s = {};
        for (auto& s : mLowMidHPState)  s = {};
        for (auto& s : mMidHighLPState) s = {};
        for (auto& s : mMidHighHPState) s = {};
        for (auto& bc : mBands)
            bc.envelope = 0.0f;
    }

    void setThresholdDb(float db) {
        for (auto& bc : mBands)
            bc.setThresholdDb(db);
    }

    void setRatio(float r) {
        for (auto& bc : mBands)
            bc.setRatio(r);
    }

    void setBandGain(int band, float gain) {
        if (band >= 0 && band < kNumBands)
            mBandGains[band] = gain;
    }

    void processBlock(float* left, float* right, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float l = left[i], r = right[i];

            // Split into 3 bands (stereo linked through same compressors)
            // Band 0: Low (LP 200Hz)
            float lowL = mLowMidLP.process(l, mLowMidLPState[0]);
            float lowR = mLowMidLP.process(r, mLowMidLPState[1]);

            // Band 1: Mid (HP 200Hz → LP 2500Hz)
            float midHighL = mLowMidHP.process(l, mLowMidHPState[0]);
            float midHighR = mLowMidHP.process(r, mLowMidHPState[1]);
            float midL = mMidHighLP.process(midHighL, mMidHighLPState[0]);
            float midR = mMidHighLP.process(midHighR, mMidHighLPState[1]);

            // Band 2: High (HP 2500Hz)
            float highL = mMidHighHP.process(midHighL, mMidHighHPState[0]);
            float highR = mMidHighHP.process(midHighR, mMidHighHPState[1]);

            // Compress each band
            lowL = mBands[0].process(lowL); lowR = mBands[0].process(lowR);
            midL = mBands[1].process(midL); midR = mBands[1].process(midR);
            highL = mBands[2].process(highL); highR = mBands[2].process(highR);

            // Per-band gain
            lowL *= mBandGains[0]; lowR *= mBandGains[0];
            midL *= mBandGains[1]; midR *= mBandGains[1];
            highL *= mBandGains[2]; highR *= mBandGains[2];

            // Recombine
            left[i] = lowL + midL + highL;
            right[i] = lowR + midR + highR;
        }
    }

private:
    double mSampleRate = 44100.0;

    LRCrossover mLowMidLP, mLowMidHP;
    LRCrossover mMidHighLP, mMidHighHP;

    // Stereo states per crossover
    LRCrossover::State mLowMidLPState[2];
    LRCrossover::State mLowMidHPState[2];
    LRCrossover::State mMidHighLPState[2];
    LRCrossover::State mMidHighHPState[2];

    std::array<BandCompressor, kNumBands> mBands;
    std::array<float, kNumBands> mBandGains = { 1.0f, 1.0f, 1.0f };
};

} // namespace dsp
