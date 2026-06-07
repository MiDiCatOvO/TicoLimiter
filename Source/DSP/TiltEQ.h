#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace dsp {

// Tilt EQ with smile curve:
// - Low shelf boost (steeper slope) + High shelf boost
// - Muddy band dynamic attenuation (200-500Hz region, user-selectable)
// - HF resonance limiter (prevents harshness)
// - 1:5 attenuation/boost ratio for muddy bands
// - Bypassed = zero processing (minimal phase change)
class TiltEQ {
public:
    static constexpr int kNumMuddyBands = 4;

    TiltEQ() = default;

    void prepare(double sampleRate) {
        float sr = static_cast<float>(sampleRate);
        bool rateChanged = std::abs(sr - mSampleRate) > 1.0f;
        mSampleRate = sr;

        struct BandInit { float freq; float Q; };
        static const BandInit bandInits[kNumMuddyBands] = {
            { 200.0f, 1.2f },
            { 280.0f, 1.2f },
            { 370.0f, 1.2f },
            { 500.0f, 1.0f }
        };

        if (rateChanged) {
            mLowShelf.setLowShelf(120.0f, 0.8f, 0.0f, mSampleRate);
            mHighShelf.setHighShelf(6000.0f, 0.7f, 0.0f, mSampleRate);
            for (int i = 0; i < kNumMuddyBands; ++i) {
                mBands[i].freq = bandInits[i].freq;
                mBands[i].detector.setPeak(bandInits[i].freq, bandInits[i].Q, 0.0f, mSampleRate);
                mBands[i].filter.setPeak(bandInits[i].freq, bandInits[i].Q, 0.0f, mSampleRate);
                mBands[i].env = 0.0f;
                mBands[i].slowEnv = 0.0f;
                mBands[i].currentGain = 0.0f;
            }
        } else {
            mLowShelf.updateLowShelfCoeffs(120.0f, 0.8f, 0.0f, mSampleRate);
            mHighShelf.updateHighShelfCoeffs(6000.0f, 0.7f, 0.0f, mSampleRate);
            for (int i = 0; i < kNumMuddyBands; ++i) {
                mBands[i].detector.updatePeakCoeffs(bandInits[i].freq, bandInits[i].Q, 0.0f, mSampleRate);
                mBands[i].filter.updatePeakCoeffs(bandInits[i].freq, bandInits[i].Q, 0.0f, mSampleRate);
            }
        }
    }

    void setEnabled(bool enabled) { mEnabled = enabled; }
    bool isEnabled() const { return mEnabled; }

    void setTilt(float db) {
        mTilt = std::clamp(db, 0.0f, 6.0f);
        updateFilters();
    }

    float getTilt() const { return mTilt; }

    void setMuddyBandEnabled(int index, bool enabled) {
        if (index >= 0 && index < kNumMuddyBands)
            mBands[index].enabled = enabled;
    }

    bool isMuddyBandEnabled(int index) const {
        return (index >= 0 && index < kNumMuddyBands) ? mBands[index].enabled : false;
    }

    float getMuddyBandFreq(int index) const {
        return (index >= 0 && index < kNumMuddyBands) ? mBands[index].freq : 0.0f;
    }

    void process(float* left, float* right, int numSamples) {
        if (!mEnabled) return;

        // === Envelope coefficients ===
        float fastAtkCoeff = std::exp(-1.0f / std::max(mSampleRate * 0.005f, 1.0f));
        float fastRelCoeff = std::exp(-1.0f / std::max(mSampleRate * 0.050f, 1.0f));
        float slowCoeff = std::exp(-1.0f / std::max(mSampleRate * 0.500f, 1.0f));
        float threshOffset = 6.0f;
        float invRatio = 0.667f; // 3:1
        float depth = mTilt / 6.0f;
        float scale = 0.25f; // 25% of original

        // === PASS 1: Envelope detection ===
        for (int s = 0; s < numSamples; ++s) {
            for (int b = 0; b < kNumMuddyBands; ++b) {
                auto& band = mBands[b];
                if (!band.enabled) continue;
                float bandL = band.detector.processSample(left[s]);
                float bandR = band.detector.processSample(right[s]);
                float level = std::max(std::abs(bandL), std::abs(bandR));
                float lin = std::max(level, 1e-7f);
                float fastCoeff = (lin > band.env) ? fastAtkCoeff : fastRelCoeff;
                band.env = lin + fastCoeff * (band.env - lin);
                band.slowEnv = lin + slowCoeff * (band.slowEnv - lin);
            }
        }

        // === Compute gains: adaptive threshold compression, 25% scale ===
        for (int b = 0; b < kNumMuddyBands; ++b) {
            auto& band = mBands[b];
            if (!band.enabled) continue;

            float fastDb = 20.0f * std::log10(std::max(band.env, 1e-7f));
            float slowDb = 20.0f * std::log10(std::max(band.slowEnv, 1e-7f));
            float threshold = slowDb - threshOffset;

            float gainDb = 0.0f;
            if (fastDb > threshold && fastDb > -60.0f) {
                float overDb = fastDb - threshold;
                gainDb = -overDb * invRatio * depth * scale;
                gainDb = std::max(gainDb, -3.0f * depth);
            }

            band.currentGain = gainDb;
            band.filter.updatePeakCoeffs(band.freq, 1.2f, band.currentGain, mSampleRate);
        }

        // === PASS 2: Apply filters ===
        for (int s = 0; s < numSamples; ++s) {
            float outL = mLowShelf.processSample(left[s]);
            float outR = mLowShelf.processSample(right[s]);
            outL = mHighShelf.processSample(outL);
            outR = mHighShelf.processSample(outR);

            for (int b = 0; b < kNumMuddyBands; ++b) {
                if (!mBands[b].enabled) continue;
                outL = mBands[b].filter.processSample(outL);
                outR = mBands[b].filter.processSample(outR);
            }

            left[s] = outL;
            right[s] = outR;
        }
    }

    // Get frequency response for visualization
    float getResponseAtFreq(float freq) const {
        if (!mEnabled) return 0.0f;
        float response = mLowShelf.getResponseDbAtFreq(freq, mSampleRate)
                       + mHighShelf.getResponseDbAtFreq(freq, mSampleRate);
        for (int b = 0; b < kNumMuddyBands; ++b) {
            if (mBands[b].enabled)
                response += mBands[b].filter.getResponseDbAtFreq(freq, mSampleRate);
        }
        return response;
    }

private:
    void updateFilters() {
        mLowShelf.updateLowShelfCoeffs(120.0f, 0.8f, mTilt, mSampleRate);
        mHighShelf.updateHighShelfCoeffs(6000.0f, 0.7f, mTilt * 0.7f, mSampleRate);
    }

    // Transposed Direct Form II biquad
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;

        void setLowShelf(float freq, float Q, float gainDb, float sr) {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * 3.14159265f * freq / sr;
            float cosw = std::cos(w0), sinw = std::sin(w0);
            float alpha = sinw / (2.0f * Q);
            float sq = 2.0f * std::sqrt(A) * alpha;
            float Ap1 = A + 1, Am1 = A - 1;
            float b0c = A * (Ap1 - Am1 * cosw + sq);
            float b1c = 2.0f * A * (Am1 - Ap1 * cosw);
            float b2c = A * (Ap1 - Am1 * cosw - sq);
            float a0c = Ap1 + Am1 * cosw + sq;
            float a1c = -2.0f * (Am1 + Ap1 * cosw);
            float a2c = Ap1 + Am1 * cosw - sq;
            b0 = b0c/a0c; b1 = b1c/a0c; b2 = b2c/a0c; a1 = a1c/a0c; a2 = a2c/a0c;
            z1 = z2 = 0;
        }

        void setHighShelf(float freq, float Q, float gainDb, float sr) {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * 3.14159265f * freq / sr;
            float cosw = std::cos(w0), sinw = std::sin(w0);
            float alpha = sinw / (2.0f * Q);
            float sq = 2.0f * std::sqrt(A) * alpha;
            float Ap1 = A + 1, Am1 = A - 1;
            float b0c = A * (Ap1 + Am1 * cosw + sq);
            float b1c = -2.0f * A * (Am1 + Ap1 * cosw);
            float b2c = A * (Ap1 + Am1 * cosw - sq);
            float a0c = Ap1 - Am1 * cosw + sq;
            float a1c = 2.0f * (Am1 - Ap1 * cosw);
            float a2c = Ap1 - Am1 * cosw - sq;
            b0 = b0c/a0c; b1 = b1c/a0c; b2 = b2c/a0c; a1 = a1c/a0c; a2 = a2c/a0c;
            z1 = z2 = 0;
        }

        void setPeak(float freq, float Q, float gainDb, float sr) {
            updatePeakCoeffs(freq, Q, gainDb, sr);
            z1 = z2 = 0;
        }

        // Update coefficients WITHOUT resetting filter state (prevents clicks)
        void updatePeakCoeffs(float freq, float Q, float gainDb, float sr) {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * 3.14159265f * freq / sr;
            float cosw = std::cos(w0), sinw = std::sin(w0);
            float alpha = sinw / (2.0f * Q);
            float b0c = 1.0f + alpha * A;
            float b1c = -2.0f * cosw;
            float b2c = 1.0f - alpha * A;
            float a0c = 1.0f + alpha / A;
            float a1c = -2.0f * cosw;
            float a2c = 1.0f - alpha / A;
            b0 = b0c/a0c; b1 = b1c/a0c; b2 = b2c/a0c; a1 = a1c/a0c; a2 = a2c/a0c;
        }

        void updateLowShelfCoeffs(float freq, float Q, float gainDb, float sr) {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * 3.14159265f * freq / sr;
            float cosw = std::cos(w0), sinw = std::sin(w0);
            float alpha = sinw / (2.0f * Q);
            float sq = 2.0f * std::sqrt(A) * alpha;
            float Ap1 = A + 1, Am1 = A - 1;
            float b0c = A * (Ap1 - Am1 * cosw + sq);
            float b1c = 2.0f * A * (Am1 - Ap1 * cosw);
            float b2c = A * (Ap1 - Am1 * cosw - sq);
            float a0c = Ap1 + Am1 * cosw + sq;
            float a1c = -2.0f * (Am1 + Ap1 * cosw);
            float a2c = Ap1 + Am1 * cosw - sq;
            b0 = b0c/a0c; b1 = b1c/a0c; b2 = b2c/a0c; a1 = a1c/a0c; a2 = a2c/a0c;
        }

        void updateHighShelfCoeffs(float freq, float Q, float gainDb, float sr) {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * 3.14159265f * freq / sr;
            float cosw = std::cos(w0), sinw = std::sin(w0);
            float alpha = sinw / (2.0f * Q);
            float sq = 2.0f * std::sqrt(A) * alpha;
            float Ap1 = A + 1, Am1 = A - 1;
            float b0c = A * (Ap1 + Am1 * cosw + sq);
            float b1c = -2.0f * A * (Am1 + Ap1 * cosw);
            float b2c = A * (Ap1 + Am1 * cosw - sq);
            float a0c = Ap1 - Am1 * cosw + sq;
            float a1c = 2.0f * (Am1 - Ap1 * cosw);
            float a2c = Ap1 - Am1 * cosw - sq;
            b0 = b0c/a0c; b1 = b1c/a0c; b2 = b2c/a0c; a1 = a1c/a0c; a2 = a2c/a0c;
        }

        float processSample(float x) {
            float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        float getResponseDbAtFreq(float freq, float sr = 48000.0f) const {
            float w = 2.0f * 3.14159265f * freq / sr;
            float cosw = std::cos(w), sinw = std::sin(w);
            float cos2w = std::cos(2.0f * w), sin2w = std::sin(2.0f * w);
            float numReal = b0 + b1 * cosw + b2 * cos2w;
            float numImag = -b1 * sinw - b2 * sin2w;
            float denReal = 1.0f + a1 * cosw + a2 * cos2w;
            float denImag = -a1 * sinw - a2 * sin2w;
            float numMag = std::sqrt(numReal * numReal + numImag * numImag);
            float denMag = std::sqrt(denReal * denReal + denImag * denImag);
            return 20.0f * std::log10(std::max(numMag / std::max(denMag, 1e-10f), 1e-10f));
        }
    };

    struct MuddyBand {
        Biquad detector;
        Biquad filter;
        float freq = 300.0f;
        float env = 0.0f;       // fast envelope (transient tracking)
        float slowEnv = 0.0f;   // slow envelope (loudness tracking)
        float currentGain = 0.0f;
        bool enabled = false;
    };

    float mSampleRate = 48000.0f;
    float mTilt = 0.0f;
    bool mEnabled = false;

    Biquad mLowShelf;
    Biquad mHighShelf;

    MuddyBand mBands[kNumMuddyBands];
};

} // namespace dsp
