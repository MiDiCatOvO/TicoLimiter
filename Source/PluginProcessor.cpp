#include "PluginProcessor.h"
#include "PluginEditor.h"

TicoLimiterProcessor::TicoLimiterProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , mParameters(*this, nullptr, juce::Identifier("TicoLimiter"), createParameterLayout())
{
    mReleaseParam      = mParameters.getRawParameterValue("release");
    mInputGainParam    = mParameters.getRawParameterValue("inputGain");
    mCeilingParam      = mParameters.getRawParameterValue("ceiling");
    mMixParam          = mParameters.getRawParameterValue("mix");
    mSaturationOnParam = mParameters.getRawParameterValue("saturationOn");
    mOddEvenMixParam   = mParameters.getRawParameterValue("oddEvenMix");
    mDriveParam        = mParameters.getRawParameterValue("drive");
    mSoftClipOnParam   = mParameters.getRawParameterValue("softClipOn");
    mRatioParam        = mParameters.getRawParameterValue("ratio");
    mOversamplingParam = mParameters.getRawParameterValue("oversampling");
    mSampleRateParam   = mParameters.getRawParameterValue("sampleRate");
    mLookAheadParam    = mParameters.getRawParameterValue("lookAhead");
    mTruePeakParam     = mParameters.getRawParameterValue("truePeak");
    mAutoReleaseParam  = mParameters.getRawParameterValue("autoRelease");
    mTiltOnParam       = mParameters.getRawParameterValue("tiltOn");
    mTiltAmountParam   = mParameters.getRawParameterValue("tiltAmount");
    mTiltMuddyParam    = mParameters.getRawParameterValue("tiltMuddy");
    mStereoLinkParam   = mParameters.getRawParameterValue("stereoLink");
}

TicoLimiterProcessor::~TicoLimiterProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout TicoLimiterProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1}, "Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lookAhead", 1}, "Look-Ahead",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputGain", 1}, "Tico Power",
        juce::NormalisableRange<float>(0.0f, 30.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"ceiling", 1}, "Ceiling",
        juce::StringArray{ "-0.1 dB", "-0.3 dB", "-0.5 dB", "-1 dB", "-3 dB" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"saturationOn", 1}, "Saturation", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"oddEvenMix", 1}, "Odd/Even",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1}, "Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"softClipOn", 1}, "Soft Clip", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"truePeak", 1}, "True Peak", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"autoRelease", 1}, "Auto Release", true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"ratio", 1}, "Ratio",
        juce::StringArray{ "1:2", "1:3", "1:5", "1:8" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"oversampling", 1}, "Oversampling",
        juce::StringArray{ "2x", "4x", "8x", "16x", "32x", "64x", "128x", "256x", "512x", "1024x", "2048x" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"sampleRate", 1}, "Sample Rate",
        juce::StringArray{ "44100", "48000", "88200", "96000" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"tiltOn", 1}, "Tico Magic", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tiltAmount", 1}, "Tilt Amount",
        juce::NormalisableRange<float>(0.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    // Muddy band selection: bitmask (bit0=200Hz, bit1=280Hz, bit2=370Hz, bit3=500Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tiltMuddy", 1}, "Tilt Muddy",
        juce::NormalisableRange<float>(0.0f, 15.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stereoLink", 1}, "Stereo Link",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    return { params.begin(), params.end() };
}

void TicoLimiterProcessor::prepareLimiter() {
    int srChoice = static_cast<int>(mSampleRateParam->load());
    static const double srTable[] = { 44100.0, 48000.0, 88200.0, 96000.0 };
    mEffectiveSampleRate = srTable[std::clamp(srChoice, 0, 3)];

    int osIndex = static_cast<int>(mOversamplingParam->load());
    bool truePeak = mTruePeakParam->load() > 0.5f;
    int effectiveOsIndex = truePeak ? std::max(osIndex, 1) : osIndex;
    int osFactor = 2 << effectiveOsIndex;
    mLimiter.prepare(mEffectiveSampleRate * osFactor);
    mLimiter.setLookAheadMs(mLookAheadParam->load());

    // Report initial latency to host
    int latencyAtHostRate = static_cast<int>(std::round(mLimiter.getLatencySamples() / static_cast<float>(osFactor)));
    setLatencySamples(latencyAtHostRate);

    mLastOversampling = effectiveOsIndex;
    mLastSampleRateChoice = srChoice;
}

void TicoLimiterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    mHostSampleRate = sampleRate;
    mSamplesPerBlock = samplesPerBlock;

    mSaturatorL.prepare(sampleRate);
    mSaturatorR.prepare(sampleRate);
    mTiltEQ.prepare(sampleRate);

    // Create JUCE oversamplers (one per quality level)
    // Parameters: numChannels, numStages, filterType, maxBlockSize (unused if 0)
    mOversampler2x  = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler4x  = std::make_unique<juce::dsp::Oversampling<float>>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler8x  = std::make_unique<juce::dsp::Oversampling<float>>(2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler16x = std::make_unique<juce::dsp::Oversampling<float>>(2, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler32x = std::make_unique<juce::dsp::Oversampling<float>>(2, 5, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler64x   = std::make_unique<juce::dsp::Oversampling<float>>(2, 6,  juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler128x  = std::make_unique<juce::dsp::Oversampling<float>>(2, 7,  juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler256x  = std::make_unique<juce::dsp::Oversampling<float>>(2, 8,  juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler512x  = std::make_unique<juce::dsp::Oversampling<float>>(2, 9,  juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler1024x = std::make_unique<juce::dsp::Oversampling<float>>(2, 10, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    mOversampler2048x = std::make_unique<juce::dsp::Oversampling<float>>(2, 11, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);

    mOversampler2x->initProcessing(samplesPerBlock);
    mOversampler4x->initProcessing(samplesPerBlock);
    mOversampler8x->initProcessing(samplesPerBlock);
    mOversampler16x->initProcessing(samplesPerBlock);
    mOversampler32x->initProcessing(samplesPerBlock);
    mOversampler64x->initProcessing(samplesPerBlock);
    mOversampler128x->initProcessing(samplesPerBlock);
    mOversampler256x->initProcessing(samplesPerBlock);
    mOversampler512x->initProcessing(samplesPerBlock);
    mOversampler1024x->initProcessing(samplesPerBlock);
    mOversampler2048x->initProcessing(samplesPerBlock);

    prepareLimiter();

    mSmoothInputGain = dbToGain(mInputGainParam->load());
    // Gain smoothing: ~10ms time constant, sample-rate invariant
    mGainSmoothCoeff = std::exp(-1.0f / std::max(static_cast<float>(sampleRate) * 0.01f, 1.0f));
    mInputLufsMeter.prepare(sampleRate);
    mOutputLufsMeter.prepare(sampleRate);

    mInputSpectrum.fill(-80.0f);
    mOutputSpectrum.fill(-80.0f);
    mInputFFTMagnitudes.fill(-80.0f);
    mOutputFFTMagnitudes.fill(-80.0f);
    mFFTBufferPos = 0;
}

void TicoLimiterProcessor::releaseResources() {
    mOversampler2x.reset();
    mOversampler4x.reset();
    mOversampler8x.reset();
    mOversampler16x.reset();
    mOversampler32x.reset();
    mOversampler64x.reset();
    mOversampler128x.reset();
    mOversampler256x.reset();
    mOversampler512x.reset();
    mOversampler1024x.reset();
    mOversampler2048x.reset();
}

void TicoLimiterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    if (numSamples == 0 || numChannels < 1) return;

    // Read parameters
    float releaseMs     = mReleaseParam->load();
    float inputGainDb   = mInputGainParam->load();
    int   ceilingIndex  = static_cast<int>(mCeilingParam->load());
    static const float ceilingDbTable[] = { -0.1f, -0.3f, -0.5f, -1.0f, -3.0f };
    float ceilingDb     = ceilingDbTable[std::clamp(ceilingIndex, 0, 4)];
    float mixPercent    = mMixParam->load();
    bool  satOn         = mSaturationOnParam->load() > 0.5f;
    float oddEvenMix    = mOddEvenMixParam->load() * 0.01f;
    float drive         = mDriveParam->load() * 0.01f;
    bool  softClipOn    = mSoftClipOnParam->load() > 0.5f;
    bool  truePeak      = mTruePeakParam->load() > 0.5f;
    bool  autoRelease   = mAutoReleaseParam->load() > 0.5f;
    int   ratioIndex    = static_cast<int>(mRatioParam->load());
    float stereoLink    = mStereoLinkParam->load() * 0.01f;
    int   osIndex       = static_cast<int>(mOversamplingParam->load());
    int   srChoice      = static_cast<int>(mSampleRateParam->load());

    // True Peak: force minimum 4x oversampling to catch inter-sample peaks
    int effectiveOsIndex = truePeak ? std::max(osIndex, 1) : osIndex;

    if (effectiveOsIndex != mLastOversampling || srChoice != mLastSampleRateChoice) {
        prepareLimiter();
        mCrossfadeRemaining = kCrossfadeLen;
    }

    // Update look-ahead time and report latency to host
    float lookAheadMs = mLookAheadParam->load();
    mLimiter.setLookAheadMs(lookAheadMs);
    {
        int osFactor = 2 << effectiveOsIndex;
        int latencyAtHostRate = static_cast<int>(std::round(mLimiter.getLatencySamples() / static_cast<float>(osFactor)));
        if (latencyAtHostRate != getLatencySamples())
            setLatencySamples(latencyAtHostRate);
    }

    // === Input spectrum (BEFORE processing) - FFT based ===
    {
        const float* data = buffer.getReadPointer(0);
        for (int s = 0; s < numSamples; ++s) {
            mFFTBuffer[mFFTBufferPos] = data[s];
            mFFTBufferPos++;

            if (mFFTBufferPos >= kFFTSize) {
                // Perform FFT
                mFFT.performRealOnlyForwardTransform(mFFTBuffer.data(), true);

                // Calculate magnitudes
                for (int bin = 0; bin < kFFTSize / 2; ++bin) {
                    float real = mFFTBuffer[bin * 2];
                    float imag = mFFTBuffer[bin * 2 + 1];
                    float magnitude = std::sqrt(real * real + imag * imag) / static_cast<float>(kFFTSize);
                    float db = magnitude > 0.00001f ? 20.0f * std::log10(magnitude) : -100.0f;
                    mInputFFTMagnitudes[bin] = mInputFFTMagnitudes[bin] * 0.7f + db * 0.3f;
                }

                // Map FFT bins to spectrum display bins (log-spaced 20 Hz – 20 kHz)
                {
                    float logMin = std::log10(20.0f);
                    float logMax = std::log10(20000.0f);
                    float fftBinWidth = static_cast<float>(mEffectiveSampleRate) / static_cast<float>(kFFTSize);
                    for (int displayBin = 0; displayBin < kSpectrumBins; ++displayBin) {
                        float t = static_cast<float>(displayBin) / static_cast<float>(kSpectrumBins - 1);
                        float freq = std::pow(10.0f, logMin + t * (logMax - logMin));
                        float fftBinFloat = freq / fftBinWidth;
                        int fftBinLow = static_cast<int>(fftBinFloat);
                        float frac = fftBinFloat - static_cast<float>(fftBinLow);
                        if (fftBinLow >= 0 && fftBinLow < kFFTSize / 2 - 1) {
                            mInputSpectrum[displayBin] = mInputFFTMagnitudes[fftBinLow]
                                                       + frac * (mInputFFTMagnitudes[fftBinLow + 1] - mInputFFTMagnitudes[fftBinLow]);
                        } else if (fftBinLow >= 0 && fftBinLow < kFFTSize / 2) {
                            mInputSpectrum[displayBin] = mInputFFTMagnitudes[fftBinLow];
                        }
                    }
                }

                mFFTBufferPos = 0;
            }
        }
    }

    // === Input metering ===
    {
        float maxLevel = 0.0f;
        double sumSq = 0.0;
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s) {
                float v = data[s];
                maxLevel = std::max(maxLevel, std::abs(v));
                sumSq += static_cast<double>(v) * v;
            }
        }
        mInputLevelDb.store(gainToDb(maxLevel));
        float rms = static_cast<float>(std::sqrt(sumSq / std::max(numSamples * numChannels, 1)));
        mInputRmsDb.store(gainToDb(rms));
    }

    // === Input LUFS ===
    {
        const float* chPtrs[2];
        chPtrs[0] = buffer.getReadPointer(0);
        chPtrs[1] = numChannels >= 2 ? buffer.getReadPointer(1) : chPtrs[0];
        mInputLufsMeter.process(chPtrs, numChannels, numSamples);
    }

    // Get dry signal for mix
    juce::AudioBuffer<float> dryBuffer;
    if (mixPercent < 99.9f)
        dryBuffer.makeCopyOf(buffer);

    // === Input gain ===
    float targetInputGain = dbToGain(inputGainDb);
    float mix = mixPercent * 0.01f;

    for (int ch = 0; ch < numChannels; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s) {
            mSmoothInputGain = targetInputGain + mGainSmoothCoeff * (mSmoothInputGain - targetInputGain);
            data[s] *= mSmoothInputGain;
        }
    }

    // === Saturation ===
    if (satOn && drive > 0.001f) {
        mSaturatorL.setDrive(drive);
        mSaturatorR.setDrive(drive);
        mSaturatorL.setOddEvenMix(oddEvenMix);
        mSaturatorR.setOddEvenMix(oddEvenMix);

        auto* dataL = buffer.getWritePointer(0);
        auto* dataR = buffer.getWritePointer(numChannels >= 2 ? 1 : 0);
        for (int s = 0; s < numSamples; ++s) {
            float L = dataL[s], R = dataR[s];
            mSaturatorL.processStereo(L, R);
            dataL[s] = L;
            if (numChannels >= 2) dataR[s] = R;
        }
    }

    // === Tico Magic (before oversampling — EQ doesn't need oversampled domain) ===
    {
        bool tiltOn = mTiltOnParam->load() > 0.5f;
        mTiltEQ.setEnabled(tiltOn);
        if (tiltOn) {
            mTiltEQ.prepare(mEffectiveSampleRate);
            mTiltEQ.setTilt(mTiltAmountParam->load());

            int muddyMask = static_cast<int>(mTiltMuddyParam->load());
            for (int b = 0; b < dsp::TiltEQ::kNumMuddyBands; ++b)
                mTiltEQ.setMuddyBandEnabled(b, (muddyMask & (1 << b)) != 0);

            auto* dataL = buffer.getWritePointer(0);
            auto* dataR = buffer.getWritePointer(numChannels >= 2 ? 1 : 0);
            mTiltEQ.process(dataL, dataR, numSamples);
        }
    }

    // === Oversample -> Limiter -> Downsample (using JUCE dsp::Oversampling) ===
    {
        // Select oversampler (effectiveOsIndex accounts for True Peak minimum)
        juce::dsp::Oversampling<float>* os = nullptr;
        switch (effectiveOsIndex) {
            case 0: os = mOversampler2x.get();  break;
            case 1: os = mOversampler4x.get();  break;
            case 2: os = mOversampler8x.get();  break;
            case 3: os = mOversampler16x.get();  break;
            case 4: os = mOversampler32x.get();  break;
            case 5: os = mOversampler64x.get();   break;
            case 6: os = mOversampler128x.get();  break;
            case 7: os = mOversampler256x.get();  break;
            case 8: os = mOversampler512x.get();  break;
            case 9: os = mOversampler1024x.get(); break;
            default: os = mOversampler2048x.get(); break;
        }

        if (os != nullptr) {
            // Create audio block from buffer
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::AudioBlock<float> subBlock = block.getSubBlock(0, static_cast<size_t>(numSamples));

            // Upsample
            auto osBlock = os->processSamplesUp(subBlock);
            int osNumSamples = static_cast<int>(osBlock.getNumSamples());

            float* osL = osBlock.getChannelPointer(0);
            float* osR = osBlock.getChannelPointer(numChannels >= 2 ? 1 : 0);

            // Soft clip BEFORE limiter.
            // The clipper attenuates excess above threshold by 1/(R+1):
            //   1:2 → clip 1/3 of excess, 1:3 → clip 1/4, 1:8 → clip 1/9.
            // The limiter then brings the signal down to the ceiling.
            float clipperGrDb = 0.0f;
            if (softClipOn) {
                float clipperThreshold = std::pow(10.0f, ceilingDb / 20.0f);

                static const float ratioValues[] = { 2.0f, 3.0f, 5.0f, 8.0f };
                float ratio = ratioValues[std::clamp(ratioIndex, 0, 3)];
                // Attenuate excess by 1/(R+1): 1:2→1/3, 1:3→1/4, 1:8→1/9
                float clipperCoeff = 1.0f / (1.0f + ratio);

                float clipperGrSum = 0.0f;
                int clipperGrCount = 0;

                for (int i = 0; i < osNumSamples; ++i) {
                    float absL = std::abs(osL[i]);
                    float absR = std::abs(osR[i]);
                    float linkedPeak = std::max(absL, absR);

                    if (linkedPeak > clipperThreshold) {
                        float overRatio = linkedPeak / clipperThreshold;
                        // Hard knee: near-instant transition
                        float kneeBlend = std::tanh((overRatio - 1.0f) * 30.0f);

                        // Linked gain: attenuate excess by clipperCoeff
                        float linkedGain = 1.0f + kneeBlend * (std::pow(overRatio, -clipperCoeff) - 1.0f);

                        // Independent gains: each channel based on its own level
                        float gainL = linkedGain;
                        if (absL > clipperThreshold) {
                            float overL = absL / clipperThreshold;
                            float kneeL = std::tanh((overL - 1.0f) * 30.0f);
                            gainL = 1.0f + kneeL * (std::pow(overL, -clipperCoeff) - 1.0f);
                        }

                        float gainR = linkedGain;
                        if (absR > clipperThreshold) {
                            float overR = absR / clipperThreshold;
                            float kneeR = std::tanh((overR - 1.0f) * 30.0f);
                            gainR = 1.0f + kneeR * (std::pow(overR, -clipperCoeff) - 1.0f);
                        }

                        // Blend: stereoLink=1 → fully linked, stereoLink=0 → fully independent
                        float finalGainL = gainL + stereoLink * (linkedGain - gainL);
                        float finalGainR = gainR + stereoLink * (linkedGain - gainR);

                        osL[i] *= finalGainL;
                        osR[i] *= finalGainR;

                        clipperGrSum += (finalGainL + finalGainR) * 0.5f;
                        clipperGrCount++;
                    }
                }

                if (clipperGrCount > 0) {
                    float avgGain = clipperGrSum / clipperGrCount;
                    clipperGrDb = 20.0f * std::log10(std::max(avgGain, 0.0001f));
                }
            }
            mClipperGainReductionDb.store(clipperGrDb);

            // Then apply limiter
            mLimiter.setThresholdDb(ceilingDb);
            mLimiter.setReleaseMs(releaseMs);
            mLimiter.setAutoRelease(autoRelease);
            mLimiter.processBlock(osL, osR, osNumSamples);

            // True peak metering (after limiter, before downsample)
            {
                float tpMax = 0.0f;
                for (int i = 0; i < osNumSamples; ++i) {
                    tpMax = std::max(tpMax, std::abs(osL[i]));
                    tpMax = std::max(tpMax, std::abs(osR[i]));
                }
                float tpDb = gainToDb(tpMax);
                mTruePeakDb.store(tpDb);
                // Track max peak (slow release so it holds)
                float prevMax = mTruePeakMaxDb.load();
                if (tpDb > prevMax)
                    mTruePeakMaxDb.store(tpDb);
                else
                    mTruePeakMaxDb.store(prevMax + 0.001f * (tpDb - prevMax)); // very slow release
            }

            // Downsample (writes back to the original buffer)
            os->processSamplesDown(subBlock);
        }

        // Safety clamp at host sample rate — catches any overshoot introduced
        // by the downsampling polyphase IIR filter after the limiter.
        {
            float ceilingLin = std::pow(10.0f, ceilingDb / 20.0f);
            auto* dataL = buffer.getWritePointer(0);
            auto* dataR = buffer.getWritePointer(numChannels >= 2 ? 1 : 0);
            for (int s = 0; s < numSamples; ++s) {
                float peak = std::max(std::abs(dataL[s]), std::abs(dataR[s]));
                if (peak > ceilingLin) {
                    float g = ceilingLin / peak;
                    dataL[s] *= g;
                    dataR[s] *= g;
                }
            }
        }
    }

    // === Crossfade to smooth oversampling transitions ===
    if (mCrossfadeRemaining > 0) {
        int fadeLen = std::min({numSamples, mCrossfadeRemaining, kCrossfadeLen});
        auto* dataL = buffer.getWritePointer(0);
        auto* dataR = buffer.getWritePointer(numChannels >= 2 ? 1 : 0);
        for (int s = 0; s < fadeLen; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(fadeLen);
            // t=0 → use saved tail (old), t=1 → use new output
            dataL[s] = mCrossfadeBufL[s] * (1.0f - t) + dataL[s] * t;
            dataR[s] = mCrossfadeBufR[s] * (1.0f - t) + dataR[s] * t;
        }
        mCrossfadeRemaining -= fadeLen;
    }

    // === Mix dry/wet ===
    if (mix < 0.999f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s)
                wet[s] = dry[s] * (1.0f - mix) + wet[s] * mix;
        }
    }

    // Final safety clamp — guarantees output never exceeds ceiling.
    // Catches overshoot from crossfade, dry/wet mix, or any residual
    // from the oversampling filter chain.
    {
        float ceilingLin = std::pow(10.0f, ceilingDb / 20.0f);
        auto* dataL = buffer.getWritePointer(0);
        auto* dataR = buffer.getWritePointer(numChannels >= 2 ? 1 : 0);
        for (int s = 0; s < numSamples; ++s) {
            float peak = std::max(std::abs(dataL[s]), std::abs(dataR[s]));
            if (peak > ceilingLin) {
                float g = ceilingLin / peak;
                dataL[s] *= g;
                dataR[s] *= g;
            }
        }
    }

    // === Output metering + spectrum ===
    {
        float maxLevel = 0.0f;
        const float* data = buffer.getReadPointer(0);
        const int bandSize = std::max(numSamples / kSpectrumBins, 1);

        double sumSq = 0.0;
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* chData = buffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s) {
                float v = chData[s];
                maxLevel = std::max(maxLevel, std::abs(v));
                sumSq += static_cast<double>(v) * v;
            }
        }
        mOutputLevelDb.store(gainToDb(maxLevel));
        float rms = static_cast<float>(std::sqrt(sumSq / std::max(numSamples * numChannels, 1)));
        mOutputRmsDb.store(gainToDb(rms));
        mGainReductionDb.store(mLimiter.getGainReductionDb());

        // Output LUFS
        {
            const float* chPtrs[2];
            chPtrs[0] = buffer.getReadPointer(0);
            chPtrs[1] = numChannels >= 2 ? buffer.getReadPointer(1) : chPtrs[0];
            mOutputLufsMeter.process(chPtrs, numChannels, numSamples);
        }

        // Output spectrum - FFT based
        static std::array<float, kFFTSize * 2> outputFFTBuffer{};
        static int outputFFTBufferPos = 0;

        for (int s = 0; s < numSamples; ++s) {
            outputFFTBuffer[outputFFTBufferPos] = data[s];
            outputFFTBufferPos++;

            if (outputFFTBufferPos >= kFFTSize) {
                // Perform FFT
                mFFT.performRealOnlyForwardTransform(outputFFTBuffer.data(), true);

                // Calculate magnitudes
                for (int bin = 0; bin < kFFTSize / 2; ++bin) {
                    float real = outputFFTBuffer[bin * 2];
                    float imag = outputFFTBuffer[bin * 2 + 1];
                    float magnitude = std::sqrt(real * real + imag * imag) / static_cast<float>(kFFTSize);
                    float db = magnitude > 0.00001f ? 20.0f * std::log10(magnitude) : -100.0f;
                    mOutputFFTMagnitudes[bin] = mOutputFFTMagnitudes[bin] * 0.7f + db * 0.3f;
                }

                // Map FFT bins to spectrum display bins (log-spaced 20 Hz – 20 kHz)
                {
                    float logMin = std::log10(20.0f);
                    float logMax = std::log10(20000.0f);
                    float fftBinWidth = static_cast<float>(mEffectiveSampleRate) / static_cast<float>(kFFTSize);
                    for (int displayBin = 0; displayBin < kSpectrumBins; ++displayBin) {
                        float t = static_cast<float>(displayBin) / static_cast<float>(kSpectrumBins - 1);
                        float freq = std::pow(10.0f, logMin + t * (logMax - logMin));
                        float fftBinFloat = freq / fftBinWidth;
                        int fftBinLow = static_cast<int>(fftBinFloat);
                        float frac = fftBinFloat - static_cast<float>(fftBinLow);
                        if (fftBinLow >= 0 && fftBinLow < kFFTSize / 2 - 1) {
                            mOutputSpectrum[displayBin] = mOutputFFTMagnitudes[fftBinLow]
                                                        + frac * (mOutputFFTMagnitudes[fftBinLow + 1] - mOutputFFTMagnitudes[fftBinLow]);
                        } else if (fftBinLow >= 0 && fftBinLow < kFFTSize / 2) {
                            mOutputSpectrum[displayBin] = mOutputFFTMagnitudes[fftBinLow];
                        }
                    }
                }

                outputFFTBufferPos = 0;
            }
        }
    }

    // Save tail of this block's output for crossfade on next block
    {
        int tailLen = std::min(numSamples, kCrossfadeLen);
        auto* dataL = buffer.getReadPointer(0);
        auto* dataR = buffer.getReadPointer(numChannels >= 2 ? 1 : 0);
        for (int s = 0; s < tailLen; ++s) {
            mCrossfadeBufL[s] = dataL[numSamples - tailLen + s];
            mCrossfadeBufR[s] = dataR[numSamples - tailLen + s];
        }
    }
}

void TicoLimiterProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = mParameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TicoLimiterProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(mParameters.state.getType()))
        mParameters.replaceState(juce::ValueTree::fromXml(*xml));
}

void TicoLimiterProcessor::getInputLufs(float& momentary, float& shortTerm, float& integrated) const {
    mInputLufsMeter.getLoudness(momentary, shortTerm, integrated);
}

void TicoLimiterProcessor::getOutputLufs(float& momentary, float& shortTerm, float& integrated) const {
    mOutputLufsMeter.getLoudness(momentary, shortTerm, integrated);
}

juce::AudioProcessorEditor* TicoLimiterProcessor::createEditor() {
    return new TicoLimiterEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TicoLimiterProcessor();
}
