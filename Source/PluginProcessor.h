#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/HarmonicSaturator.h"
#include "DSP/Limiter.h"
#include "DSP/TiltEQ.h"
#include "DSP/LUFSMeter.h"

class TicoLimiterProcessor : public juce::AudioProcessor {
public:
    TicoLimiterProcessor();
    ~TicoLimiterProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tico Limiter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() { return mParameters; }

    // Metering
    float getCurrentGainReductionDb() const { return mGainReductionDb.load(); }
    float getCurrentClipperGainReductionDb() const { return mClipperGainReductionDb.load(); }
    float getCurrentInputLevelDb() const { return mInputLevelDb.load(); }
    float getCurrentOutputLevelDb() const { return mOutputLevelDb.load(); }
    float getCurrentInputRmsDb() const { return mInputRmsDb.load(); }
    float getCurrentOutputRmsDb() const { return mOutputRmsDb.load(); }
    float getCurrentTruePeakDb() const { return mTruePeakDb.load(); }
    float getTruePeakMaxDb() const { return mTruePeakMaxDb.load(); }
    void resetTruePeakMax() { mTruePeakMaxDb.store(-60.0f); }
    void getInputLufs(float& momentary, float& shortTerm, float& integrated) const;
    void getOutputLufs(float& momentary, float& shortTerm, float& integrated) const;

    // Spectrum data for Analysis page
    static constexpr int kSpectrumBins = 512;
    const float* getInputSpectrum() const { return mInputSpectrum.data(); }
    const float* getOutputSpectrum() const { return mOutputSpectrum.data(); }
    int getNumSpectrumBins() const { return kSpectrumBins; }
    double getCurrentSampleRate() const { return mEffectiveSampleRate; }

    // Tilt EQ response for visualization
    float getTiltResponseAtFreq(float freq) const { return mTiltEQ.getResponseAtFreq(freq); }
    bool isTiltEnabled() const { return mTiltEQ.isEnabled(); }

    static constexpr int kNumChannels = 2;

private:
    juce::AudioProcessorValueTreeState mParameters;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepareLimiter();

    // DSP
    dsp::HarmonicSaturator mSaturatorL, mSaturatorR;
    dsp::LookAheadLimiter mLimiter;
    dsp::TiltEQ mTiltEQ;

    // JUCE oversampling (one per quality level)
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler4x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler8x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler16x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler32x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler64x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler128x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler256x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler512x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler1024x;
    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampler2048x;

    // Parameters
    std::atomic<float>* mReleaseParam = nullptr;
    std::atomic<float>* mInputGainParam = nullptr;
    std::atomic<float>* mCeilingParam = nullptr;
    std::atomic<float>* mMixParam = nullptr;
    std::atomic<float>* mSaturationOnParam = nullptr;
    std::atomic<float>* mOddEvenMixParam = nullptr;
    std::atomic<float>* mDriveParam = nullptr;
    std::atomic<float>* mSoftClipOnParam = nullptr;
    std::atomic<float>* mOversamplingParam = nullptr;
    std::atomic<float>* mSampleRateParam = nullptr;
    std::atomic<float>* mRatioParam = nullptr;
    std::atomic<float>* mLookAheadParam = nullptr;
    std::atomic<float>* mTruePeakParam = nullptr;
    std::atomic<float>* mAutoReleaseParam = nullptr;
    std::atomic<float>* mTiltOnParam = nullptr;
    std::atomic<float>* mTiltAmountParam = nullptr;
    std::atomic<float>* mTiltMuddyParam = nullptr;
    std::atomic<float>* mStereoLinkParam = nullptr;

    // Metering
    std::atomic<float> mGainReductionDb{ 0.0f };
    std::atomic<float> mClipperGainReductionDb{ 0.0f };
    std::atomic<float> mInputLevelDb{ -60.0f };
    std::atomic<float> mOutputLevelDb{ -60.0f };
    std::atomic<float> mInputRmsDb{ -60.0f };
    std::atomic<float> mOutputRmsDb{ -60.0f };
    std::atomic<float> mTruePeakDb{ -60.0f };
    std::atomic<float> mTruePeakMaxDb{ -60.0f };
    LUFSMeter mInputLufsMeter;
    LUFSMeter mOutputLufsMeter;

    float mSmoothInputGain = 1.0f;
    float mGainSmoothCoeff = 0.999f; // recalculated in prepareToPlay

    double mHostSampleRate = 44100.0;
    double mEffectiveSampleRate = 44100.0;
    int mLastOversampling = -1;
    int mLastSampleRateChoice = -1;

    // Crossfade buffer for smooth oversampling transitions
    static constexpr int kCrossfadeLen = 64;
    std::array<float, kCrossfadeLen> mCrossfadeBufL{};
    std::array<float, kCrossfadeLen> mCrossfadeBufR{};
    int mCrossfadePos = 0;
    int mCrossfadeRemaining = 0;
    int mSamplesPerBlock = 512;

    std::array<float, kSpectrumBins> mInputSpectrum{};
    std::array<float, kSpectrumBins> mOutputSpectrum{};

    // FFT for spectrum analysis
    static constexpr int kFFTOrder = 10;  // 2^10 = 1024 bins
    static constexpr int kFFTSize = 1 << kFFTOrder;
    juce::dsp::FFT mFFT{ kFFTOrder };
    std::array<float, kFFTSize * 2> mFFTBuffer{};
    std::array<float, kFFTSize> mInputFFTMagnitudes{};
    std::array<float, kFFTSize> mOutputFFTMagnitudes{};
    int mFFTBufferPos = 0;

    static float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }
    static float gainToDb(float g) { return g > 0.0001f ? 20.0f * std::log10(g) : -80.0f; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TicoLimiterProcessor)
};
