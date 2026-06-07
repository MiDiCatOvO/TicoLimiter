#pragma once
#include "PluginProcessor.h"
#include "GUI/KawaiiLookAndFeel.h"
#include "GUI/MeterComponent.h"
#include "GUI/SpectrumAnalyzer.h"
#include "GUI/GRHistoryDisplay.h"
#include "GUI/LufsDisplay.h"
#include "GUI/ClipperLED.h"
#include "GUI/RainbowSliderLookAndFeel.h"
#include "PresetManager.h"
#include <juce_audio_processors/juce_audio_processors.h>

class TicoLimiterEditor : public juce::AudioProcessorEditor,
                           private juce::Timer {
public:
    TicoLimiterEditor(TicoLimiterProcessor& processor);
    ~TicoLimiterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& name,
                   const juce::String& suffix, double min, double max, double step, double def);
    void refreshPresetCombo();

    TicoLimiterProcessor& mProcessor;
    KawaiiLookAndFeel mLookAndFeel;
    RainbowSliderLookAndFeel mRainbowSliderLAF;

    // === Page system ===
    enum class Page { Controls, Analysis };
    Page mCurrentPage = Page::Controls;

    // Tab buttons
    juce::TextButton mControlsTabButton{ "Controls" };
    juce::TextButton mAnalysisTabButton{ "Analysis" };

    // Title
    juce::Label mTitleLabel;
    juce::Label mSubtitleLabel;

    // Preset
    std::unique_ptr<PresetManager> mPresetManager;
    juce::ComboBox mPresetCombo;
    juce::TextButton mPresetSaveBtn{ "+" };
    juce::TextButton mPresetDeleteBtn{ "-" };

    // === Controls Page ===
    // IO Section
    juce::Slider mInputGainSlider;  juce::Label mInputGainLabel;
    juce::Slider mMixSlider;        juce::Label mMixLabel;
    juce::ComboBox mCeilingCombo;
    juce::Label    mCeilingLabel;

    // Saturation
    juce::GroupComponent mSatGroup;
    juce::ToggleButton   mSatOnButton;
    juce::Slider         mOddEvenSlider; juce::Label mOddEvenLabel;
    juce::Slider         mDriveSlider;   juce::Label mDriveLabel;

    // Soft Clip
    juce::GroupComponent mClipGroup;
    juce::ToggleButton   mClipOnButton;

    // Limiter
    juce::GroupComponent mLimiterGroup;
    juce::Slider         mReleaseSlider;   juce::Label mReleaseLabel;
    juce::Label          mReleasePctLabel;  // shown when adaptive release is on
    juce::Slider         mLookAheadSlider; juce::Label mLookAheadLabel;

    // True Peak
    juce::ToggleButton mTruePeakButton;
    juce::Label        mTruePeakHint;

    // Auto Release
    juce::ToggleButton mAutoReleaseButton;
    juce::Label        mAutoReleaseHint;

    // Clipper/Limiter Ratio
    juce::ComboBox mRatioCombo;
    juce::Label    mRatioLabel;

    // Stereo Link
    juce::Slider mStereoLinkSlider; juce::Label mStereoLinkLabel;

    // Oversampling
    juce::ComboBox mOSCombo;
    juce::Label    mOSLabel;

    // Sample Rate
    juce::ComboBox mSRCombo;
    juce::Label    mSRLabel;

    // GR meter for Controls page
    MeterComponent mGRMeter;

    // Clipper LED indicator
    ClipperLED mClipperLed;

    // === Analysis Page ===
    SpectrumAnalyzer mSpectrumAnalyzer;
    GRHistoryDisplay mGRHistoryDisplay;

    // Tilt EQ controls
    juce::ToggleButton mTiltOnButton;
    juce::Slider       mTiltAmountSlider; juce::Label mTiltAmountLabel;
    juce::ToggleButton mTiltMuddyButtons[4];
    juce::Label        mTiltMuddyFreqLabels[4];
    juce::Label        mTiltTitle;
    juce::Label        mTiltCleaningLabel;

    // Meters for Analysis page
    MeterComponent mOutputMeter;
    juce::Label    mOutputDbLabel;
    juce::Label    mOutputRmsLabel;
    juce::Label    mTruePeakLabel;
    LufsDisplay    mLufsDisplay;

    // Attachments
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SA> mInputGainAtt, mMixAtt;
    std::unique_ptr<CA> mCeilingAtt;
    std::unique_ptr<SA> mOddEvenAtt, mDriveAtt;
    std::unique_ptr<SA> mReleaseAtt, mLookAheadAtt, mStereoLinkAtt;
    std::unique_ptr<BA> mSatOnAtt, mClipOnAtt, mTruePeakAtt, mAutoReleaseAtt;
    std::unique_ptr<CA> mOSAtt, mSRAtt, mRatioAtt;
    std::unique_ptr<BA> mTiltOnAtt;
    std::unique_ptr<SA> mTiltAmountAtt;

    // Animation
    float mAnimPhase = 0.0f;
    void drawDeco(juce::Graphics& g);
    void drawAnimatedStars(juce::Graphics& g, float phase);

    // Tab drawing
    void drawTabBar(juce::Graphics& g);
    void updatePageVisibility();
    juce::Rectangle<int> getTabArea() const;
    juce::Rectangle<int> getContentArea() const;

    // Page content painting
    void paintControlsPage(juce::Graphics& g);
    void paintAnalysisPage(juce::Graphics& g);

    static constexpr int kW = 1012;
    static constexpr int kH = 780;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TicoLimiterEditor)
};
