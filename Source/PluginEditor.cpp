#include "PluginEditor.h"

TicoLimiterEditor::TicoLimiterEditor(TicoLimiterProcessor& processor)
    : AudioProcessorEditor(&processor), mProcessor(processor)
{
    setLookAndFeel(&mLookAndFeel);
    setSize(kW, kH);
    startTimerHz(30);

    // Title drawn directly in paint() to avoid LookAndFeel font override
    mTitleLabel.setVisible(false);
    mSubtitleLabel.setVisible(false);

    // Preset manager
    mPresetManager = std::make_unique<PresetManager>(mProcessor.getParameters());
    mPresetCombo.setColour(juce::ComboBox::textColourId, juce::Colour(0xFF333333));
    mPresetCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x00000000));
    mPresetCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0x00000000));
    refreshPresetCombo();
    mPresetCombo.onChange = [this]() {
        auto name = mPresetCombo.getText();
        if (name.isNotEmpty())
            mPresetManager->loadPreset(name);
    };
    addAndMakeVisible(mPresetCombo);

    mPresetSaveBtn.setColour(juce::TextButton::buttonColourId, KawaiiLookAndFeel::Colors::sakuraPink.withAlpha(0.3f));
    mPresetSaveBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF666666));
    mPresetSaveBtn.onClick = [this]() {
        auto* alert = new juce::AlertWindow("Save Preset", "Enter preset name:", juce::AlertWindow::NoIcon);
        alert->addTextEditor("name", "", "Preset name");
        alert->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result) {
            if (result == 1) {
                auto name = alert->getTextEditorContents("name").trim();
                if (name.isNotEmpty()) {
                    mPresetManager->savePreset(name);
                    refreshPresetCombo();
                }
            }
            delete alert;
        }));
    };
    addAndMakeVisible(mPresetSaveBtn);

    mPresetDeleteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x20FF0000));
    mPresetDeleteBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF999999));
    mPresetDeleteBtn.onClick = [this]() {
        auto name = mPresetCombo.getText();
        if (name.isNotEmpty() && !mPresetManager->isFactoryPreset(name)) {
            auto* alert = new juce::AlertWindow("Delete Preset",
                "Delete \"" + name + "\"?", juce::AlertWindow::WarningIcon);
            alert->addButton("Delete", 1);
            alert->addButton("Cancel", 0);
            alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert, name](int result) {
                if (result == 1) {
                    mPresetManager->deletePreset(name);
                    mPresetManager->setCurrentPreset("Default");
                    mPresetManager->loadPreset("Default");
                    refreshPresetCombo();
                }
                delete alert;
            }));
        }
    };
    addAndMakeVisible(mPresetDeleteBtn);

    // === Tab buttons ===
    mControlsTabButton.setButtonText("Controls");
    mControlsTabButton.setClickingTogglesState(false);
    mControlsTabButton.onClick = [this]() {
        mCurrentPage = Page::Controls;
        mControlsTabButton.setToggleState(true, juce::dontSendNotification);
        mAnalysisTabButton.setToggleState(false, juce::dontSendNotification);
        updatePageVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible(mControlsTabButton);

    mAnalysisTabButton.setButtonText("Analysis");
    mAnalysisTabButton.setClickingTogglesState(false);
    mAnalysisTabButton.onClick = [this]() {
        mCurrentPage = Page::Analysis;
        mControlsTabButton.setToggleState(false, juce::dontSendNotification);
        mAnalysisTabButton.setToggleState(true, juce::dontSendNotification);
        updatePageVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible(mAnalysisTabButton);

    mControlsTabButton.setToggleState(true, juce::dontSendNotification);

    // === Groups MUST be added BEFORE sliders (z-order: groups behind, sliders on top) ===
    mSatGroup.setText("Saturation");
    addAndMakeVisible(mSatGroup);

    mClipGroup.setText("Soft Clip");
    addAndMakeVisible(mClipGroup);

    mLimiterGroup.setText("Limiter");
    addAndMakeVisible(mLimiterGroup);

    // === Controls Page sliders (added AFTER groups so they render on top) ===
    setupKnob(mInputGainSlider, mInputGainLabel, "Tico Power", " dB", 0, 30, 0.1, 0);
    setupKnob(mMixSlider, mMixLabel, "Mix", " %", 0, 100, 1, 100);

    // Ceiling combo
    mCeilingCombo.addItemList({"-0.1 dB", "-0.3 dB", "-0.5 dB", "-1 dB", "-3 dB"}, 1);
    mCeilingCombo.setSelectedItemIndex(0, juce::dontSendNotification);
    mCeilingCombo.setColour(juce::ComboBox::textColourId, juce::Colour(0xFF333333));
    addAndMakeVisible(mCeilingCombo);
    mCeilingLabel.setText("Ceiling", juce::dontSendNotification);
    mCeilingLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold)));
    mCeilingLabel.setColour(juce::Label::textColourId, KawaiiLookAndFeel::Colors::labelColor);
    mCeilingLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mCeilingLabel);
    setupKnob(mOddEvenSlider, mOddEvenLabel, "Odd/Even", " %", 0, 100, 1, 50);
    setupKnob(mDriveSlider, mDriveLabel, "Drive", " %", 0, 100, 1, 0);
    setupKnob(mReleaseSlider, mReleaseLabel, "Release", " ms", 10, 500, 1, 100);
    setupKnob(mLookAheadSlider, mLookAheadLabel, "Look-Ahead", " ms", 0, 10, 0.1, 0);

    // Release percentage label (adaptive mode)
    mReleasePctLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::plain)));
    mReleasePctLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
    mReleasePctLabel.setJustificationType(juce::Justification::centred);
    mReleasePctLabel.setVisible(false);
    addAndMakeVisible(mReleasePctLabel);

    mInputGainAtt  = std::make_unique<SA>(mProcessor.getParameters(), "inputGain", mInputGainSlider);
    mMixAtt        = std::make_unique<SA>(mProcessor.getParameters(), "mix", mMixSlider);
    mCeilingAtt    = std::make_unique<CA>(mProcessor.getParameters(), "ceiling", mCeilingCombo);
    mOddEvenAtt    = std::make_unique<SA>(mProcessor.getParameters(), "oddEvenMix", mOddEvenSlider);
    mDriveAtt      = std::make_unique<SA>(mProcessor.getParameters(), "drive", mDriveSlider);
    mReleaseAtt    = std::make_unique<SA>(mProcessor.getParameters(), "release", mReleaseSlider);
    mLookAheadAtt  = std::make_unique<SA>(mProcessor.getParameters(), "lookAhead", mLookAheadSlider);

    // Toggle buttons (added AFTER groups)
    mSatOnButton.setButtonText("ON");
    addAndMakeVisible(mSatOnButton);
    mSatOnAtt = std::make_unique<BA>(mProcessor.getParameters(), "saturationOn", mSatOnButton);

    mClipOnButton.setButtonText("ON");
    addAndMakeVisible(mClipOnButton);
    mClipOnAtt = std::make_unique<BA>(mProcessor.getParameters(), "softClipOn", mClipOnButton);

    setupKnob(mStereoLinkSlider, mStereoLinkLabel, "Link", " %", 0, 100, 1, 100);
    mStereoLinkAtt = std::make_unique<SA>(mProcessor.getParameters(), "stereoLink", mStereoLinkSlider);

    // True Peak toggle — oversampled ISP detection
    mTruePeakButton.setButtonText("True Peak");
    mTruePeakButton.setTooltip("Oversampled inter-sample peak detection.\nForces min 4x oversampling to catch peaks between samples.");
    addAndMakeVisible(mTruePeakButton);
    mTruePeakAtt = std::make_unique<BA>(mProcessor.getParameters(), "truePeak", mTruePeakButton);

    // Auto Release toggle — adaptive release time
    mAutoReleaseButton.setButtonText("Auto Rel");
    mAutoReleaseButton.setTooltip("Adaptive release: fast recovery on transients,\nslow release on sustained material.");
    addAndMakeVisible(mAutoReleaseButton);
    mAutoReleaseAtt = std::make_unique<BA>(mProcessor.getParameters(), "autoRelease", mAutoReleaseButton);

    // Hint labels (small gray text below buttons)
    auto setupHint = [&](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 8.0f, juce::Font::plain)));
        label.setColour(juce::Label::textColourId, juce::Colour(0xFF999999));
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
    };
    setupHint(mTruePeakHint, "True Peak");
    setupHint(mAutoReleaseHint, "Adaptive release time");

    // OS combo - no separate label, ComboBox shows text
    mOSCombo.addItemList({"2x", "4x", "8x", "16x", "32x", "64x", "128x", "256x", "512x", "1024x", "2048x"}, 1);
    mOSCombo.setSelectedItemIndex(1, juce::dontSendNotification); // default 4x
    addAndMakeVisible(mOSCombo);
    mOSAtt = std::make_unique<CA>(mProcessor.getParameters(), "oversampling", mOSCombo);

    // Sample Rate combo - no separate label
    mSRCombo.addItemList({"44100", "48000", "88200", "96000"}, 1);
    mSRCombo.setSelectedItemIndex(1, juce::dontSendNotification); // default 48000
    addAndMakeVisible(mSRCombo);
    mSRAtt = std::make_unique<CA>(mProcessor.getParameters(), "sampleRate", mSRCombo);

    // Ratio combo - blue text (different from other combos)
    mRatioCombo.addItemList({"1:2", "1:3", "1:5", "1:8"}, 1);
    mRatioCombo.setSelectedItemIndex(2, juce::dontSendNotification); // default 1:5
    mRatioCombo.setColour(juce::ComboBox::textColourId, juce::Colour(0xFF66CCFF));  // Blue for ratio
    addAndMakeVisible(mRatioCombo);
    mRatioAtt = std::make_unique<CA>(mProcessor.getParameters(), "ratio", mRatioCombo);

    // GR meter for Controls page
    mGRMeter.setLabel("GR");
    addAndMakeVisible(mGRMeter);

    // Clipper LED
    addAndMakeVisible(mClipperLed);

    // === Analysis Page components ===
    addAndMakeVisible(mSpectrumAnalyzer);
    addAndMakeVisible(mGRHistoryDisplay);

    mOutputMeter.setLabel("OUT");
    addAndMakeVisible(mOutputMeter);

    // dB label
    mOutputDbLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold)));
    mOutputDbLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFF6B9D));
    mOutputDbLabel.setJustificationType(juce::Justification::centred);
    mOutputDbLabel.setText("-- dB", juce::dontSendNotification);
    addAndMakeVisible(mOutputDbLabel);

    // RMS label
    mOutputRmsLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold)));
    mOutputRmsLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE8658A));
    mOutputRmsLabel.setJustificationType(juce::Justification::centred);
    mOutputRmsLabel.setText("-- RMS", juce::dontSendNotification);
    addAndMakeVisible(mOutputRmsLabel);

    // True Peak label (click to reset max)
    mTruePeakLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold)));
    mTruePeakLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFFD700));
    mTruePeakLabel.setJustificationType(juce::Justification::centred);
    mTruePeakLabel.setText("-- TP", juce::dontSendNotification);
    mTruePeakLabel.setTooltip("Click to reset peak hold");
    mTruePeakLabel.setInterceptsMouseClicks(true, false);
    mTruePeakLabel.addMouseListener(this, false);
    addAndMakeVisible(mTruePeakLabel);

    // LUFS display
    addAndMakeVisible(mLufsDisplay);

    // === Tilt EQ controls ===
    mTiltTitle.setText("Tico Magic", juce::dontSendNotification);

    static const char* muddyFreqTexts[] = { "200Hz", "280Hz", "370Hz", "500Hz" };
    // Gradient red: dark → light
    static const juce::uint32 muddyFreqColours[] = { 0xFFCC2222, 0xFFDD4444, 0xFFEE6666, 0xFFFF8888 };
    for (int i = 0; i < 4; ++i) {
        mTiltMuddyFreqLabels[i].setText(muddyFreqTexts[i], juce::dontSendNotification);
        mTiltMuddyFreqLabels[i].setFont(juce::Font(juce::FontOptions("Helvetica Neue", 7.0f, juce::Font::plain)));
        mTiltMuddyFreqLabels[i].setColour(juce::Label::textColourId, juce::Colour(muddyFreqColours[i]));
        mTiltMuddyFreqLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(mTiltMuddyFreqLabels[i]);
    }

    mTiltCleaningLabel.setText("PikaPika", juce::dontSendNotification);
    mTiltCleaningLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::bold)));
    mTiltCleaningLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFFDD44)); // light yellow
    mTiltCleaningLabel.setJustificationType(juce::Justification::centredLeft);
    mTiltCleaningLabel.setLookAndFeel(nullptr); // bypass LookAndFeel font override
    addAndMakeVisible(mTiltCleaningLabel);
    mTiltTitle.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::bold)));
    mTiltTitle.setColour(juce::Label::textColourId, juce::Colour(0xFFFF6B9D)); // pink
    mTiltTitle.setLookAndFeel(nullptr); // bypass LookAndFeel font override
    addAndMakeVisible(mTiltTitle);

    mTiltOnButton.setButtonText("ON");
    addAndMakeVisible(mTiltOnButton);
    mTiltOnAtt = std::make_unique<BA>(mProcessor.getParameters(), "tiltOn", mTiltOnButton);

    mTiltAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mTiltAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 16);
    mTiltAmountSlider.setRange(0, 6, 0.1);
    mTiltAmountSlider.setValue(0);
    mTiltAmountSlider.setTextValueSuffix(" dB");
    mTiltAmountSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFF66CCFF));
    mTiltAmountSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF00FF88));
    mTiltAmountSlider.setLookAndFeel(&mRainbowSliderLAF);
    addAndMakeVisible(mTiltAmountSlider);
    mTiltAmountAtt = std::make_unique<SA>(mProcessor.getParameters(), "tiltAmount", mTiltAmountSlider);

    mTiltAmountLabel.setText("Kirakira", juce::dontSendNotification);
    mTiltAmountLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::bold)));
    mTiltAmountLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF66CCFF)); // light blue
    mTiltAmountLabel.setJustificationType(juce::Justification::centredLeft);
    mTiltAmountLabel.setLookAndFeel(nullptr); // bypass LookAndFeel font override
    addAndMakeVisible(mTiltAmountLabel);

    // Muddy band checkboxes
    static const char* muddyFreqs[] = { "200Hz", "280Hz", "370Hz", "500Hz" };
    for (int i = 0; i < 4; ++i) {
        mTiltMuddyButtons[i].setButtonText(muddyFreqs[i]);
        mTiltMuddyButtons[i].setTooltip(juce::String("Dynamic attenuate ") + muddyFreqs[i]);
        addAndMakeVisible(mTiltMuddyButtons[i]);

        int idx = i;
        mTiltMuddyButtons[i].onClick = [this, idx]() {
            auto* raw = mProcessor.getParameters().getRawParameterValue("tiltMuddy");
            int mask = raw ? static_cast<int>(raw->load()) : 0;
            if (mTiltMuddyButtons[idx].getToggleState()) mask |= (1 << idx);
            else mask &= ~(1 << idx);
            auto* p = mProcessor.getParameters().getParameter("tiltMuddy");
            if (p) p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(mask)));
        };
    }


    updatePageVisibility();
}

void TicoLimiterEditor::mouseUp(const juce::MouseEvent& event) {
    if (event.eventComponent == &mTruePeakLabel && event.mouseWasClicked())
        mProcessor.resetTruePeakMax();
}

TicoLimiterEditor::~TicoLimiterEditor() {
    stopTimer();
    mTiltAmountSlider.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void TicoLimiterEditor::timerCallback() {
    mAnimPhase += 0.05f;
    if (mAnimPhase > 6.283f) mAnimPhase -= 6.283f;

    // Dynamic release display: 0-100% when adaptive, ms when manual
    {
        auto* raw = mProcessor.getParameters().getRawParameterValue("autoRelease");
        bool adaptive = raw && raw->load() > 0.5f;
        bool isControls = (mCurrentPage == Page::Controls);
        if (adaptive && isControls) {
            int pct = static_cast<int>(std::round((mReleaseSlider.getValue() - 10.0) / 490.0 * 100.0));
            mReleasePctLabel.setText(juce::String(std::clamp(pct, 0, 100)) + " %", juce::dontSendNotification);
            mReleaseSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            mReleasePctLabel.setVisible(true);
        } else {
            mReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
            mReleaseSlider.setTextValueSuffix(" ms");
            mReleasePctLabel.setVisible(false);
        }
    }

    // Update glow intensity based on GR (0 = idle, 1 = heavy limiting)
    float glowGrDb = mProcessor.getCurrentGainReductionDb();
    mLookAndFeel.glowIntensity = std::clamp(-glowGrDb / 12.0f, 0.0f, 1.0f);

    // Update meters
    mGRMeter.setGainReductionDb(glowGrDb);
    mOutputMeter.setLevelDb(mProcessor.getCurrentOutputLevelDb());

    // Update spectrum data
    double sr = mProcessor.getCurrentSampleRate();
    mSpectrumAnalyzer.setSampleRate(sr);
    mSpectrumAnalyzer.setInputSpectrum(mProcessor.getInputSpectrum(), mProcessor.getNumSpectrumBins());
    mSpectrumAnalyzer.setOutputSpectrum(mProcessor.getOutputSpectrum(), mProcessor.getNumSpectrumBins());

    // Tilt curve hidden from spectrum display
    mSpectrumAnalyzer.setTiltCurve({}, false);

    // Update GR history
    float grDb = mProcessor.getCurrentGainReductionDb();

    // Headroom: 1.0 = no GR (full headroom), 0.0 = -12dB GR (no headroom)
    float clipperGrDb = mProcessor.getCurrentClipperGainReductionDb();
    {
        auto* raw = mProcessor.getParameters().getRawParameterValue("softClipOn");
        bool clipperOn = raw && raw->load() > 0.5f;
        mClipperLed.setClipperState(clipperGrDb, clipperOn);
    }
    mGRHistoryDisplay.setGainReduction(grDb);
    mGRHistoryDisplay.setClipperGainReduction(clipperGrDb);

    // Update labels
    float outDb = mProcessor.getCurrentOutputLevelDb();
    mOutputDbLabel.setText(juce::String(outDb, 1) + " dB", juce::dontSendNotification);

    float outRms = mProcessor.getCurrentOutputRmsDb();
    mOutputRmsLabel.setText(juce::String(outRms, 1) + " RMS", juce::dontSendNotification);

    float truePeak = mProcessor.getCurrentTruePeakDb();
    float tpMax = mProcessor.getTruePeakMaxDb();
    mTruePeakLabel.setText(juce::String(truePeak, 1) + " / " + juce::String(tpMax, 1) + " TP", juce::dontSendNotification);

    // Sync muddy band checkboxes
    {
        auto* raw = mProcessor.getParameters().getRawParameterValue("tiltMuddy");
        int mask = raw ? static_cast<int>(raw->load()) : 0;
        for (int i = 0; i < 4; ++i) {
            bool on = (mask & (1 << i)) != 0;
            if (mTiltMuddyButtons[i].getToggleState() != on)
                mTiltMuddyButtons[i].setToggleState(on, juce::dontSendNotification);
        }
    }

    // Update LUFS display
    float outMom, outSht, outInt;
    mProcessor.getOutputLufs(outMom, outSht, outInt);
    mLufsDisplay.setMomentary(outMom);
    mLufsDisplay.setShortTerm(outSht);
    mLufsDisplay.setIntegrated(outInt);

    repaint();
}

void TicoLimiterEditor::setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name,
                                    const juce::String& suffix, double min, double max, double step, double def) {
    s.setSliderStyle(juce::Slider::LinearVertical);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    s.setRange(min, max, step);
    s.setValue(def);
    s.setTextValueSuffix(suffix);
    // Force black text for value display
    s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFF333333));
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold)));
    l.setColour(juce::Label::textColourId, KawaiiLookAndFeel::Colors::labelColor);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void TicoLimiterEditor::paint(juce::Graphics& g) {
    // Soft pink-white background
    juce::ColourGradient bg(KawaiiLookAndFeel::Colors::background, 0.0f, 0.0f,
                             KawaiiLookAndFeel::Colors::backgroundPink, 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(bg);
    g.fillAll();

    drawDeco(g);
    drawAnimatedStars(g, mAnimPhase);

    // Title
    {
        auto b = getLocalBounds().reduced(22);
        auto titleArea = b.removeFromTop(100);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 52.0f, juce::Font::bold)));
        g.setColour(KawaiiLookAndFeel::Colors::titleColor);
        g.drawText("Tico Limiter", titleArea, juce::Justification::centred);
    }

    // Outer border - kawaii style
    float gi = mLookAndFeel.glowIntensity;
    auto borderBounds = getLocalBounds().toFloat().reduced(2.0f);

    // Soft glow layers
    for (int i = 2; i >= 1; --i) {
        float expand = static_cast<float>(i) * 2.0f * (1.0f + gi);
        g.setColour(KawaiiLookAndFeel::Colors::glowPink.withAlpha(0.06f * (3 - i)));
        g.drawRoundedRectangle(borderBounds.expanded(expand), 16.0f + expand, 1.5f);
    }

    // Main border - sakura pink
    g.setColour(KawaiiLookAndFeel::Colors::sakuraPink.withAlpha(0.3f + gi * 0.15f));
    g.drawRoundedRectangle(borderBounds, 16.0f, 1.5f);

    drawTabBar(g);
}

void TicoLimiterEditor::resized() {
    auto b = getLocalBounds().reduced(22);

    // Title area
    b.removeFromTop(6); // top padding
    auto titleArea = b.removeFromTop(52);
    mTitleLabel.setBounds(titleArea);
    b.removeFromTop(6); // bottom padding

    // Preset controls — right side of title
    int btnW = 24;
    int comboW = 150;
    int presetH = 24;
    int presetY = titleArea.getY() + (titleArea.getHeight() - presetH) / 2;
    mPresetDeleteBtn.setBounds(titleArea.getRight() - btnW, presetY, btnW, presetH);
    mPresetSaveBtn.setBounds(titleArea.getRight() - btnW * 2 - 4, presetY, btnW, presetH);
    mPresetCombo.setBounds(titleArea.getRight() - btnW * 2 - 4 - comboW - 6, presetY, comboW, presetH);

    b.removeFromTop(12);

    // Tab buttons
    auto tabArea = b.removeFromTop(40);
    int tabW = 112;
    int tabH = 33;
    mControlsTabButton.setBounds(tabArea.getX(), tabArea.getY(), tabW, tabH);
    mAnalysisTabButton.setBounds(tabArea.getX() + tabW + 13, tabArea.getY(), tabW, tabH);

    b.removeFromTop(12);

    // Content area
    auto content = b;

    if (mCurrentPage == Page::Controls) {
        // === Controls Page - Modular Layout ===
        int moduleGap = 10;
        int padding = 12;

        // Calculate total available height for balanced split
        int totalH = content.getHeight();
        int topH = static_cast<int>(totalH * 0.45f);   // 45% for I/O
        int bottomH = totalH - topH - moduleGap;        // rest for processing

        // Module 1: I/O Controls (top) - BIGGER
        auto ioModule = content.removeFromTop(topH);
        {
            // Left section: Faders
            int faderW = 70;
            int faderGap = 18;
            int faderStartX = ioModule.getX() + padding;
            int faderY = ioModule.getY() + 32;
            int faderH = ioModule.getHeight() - 50;  // Use most of the module height

            mInputGainSlider.setBounds(faderStartX, faderY, faderW, faderH);
            mInputGainLabel.setBounds(faderStartX, ioModule.getY() + 8, faderW, 18);

            int mixX = faderStartX + faderW + faderGap;
            mMixSlider.setBounds(mixX, faderY, faderW, faderH);
            mMixLabel.setBounds(mixX, ioModule.getY() + 8, faderW, 18);

            // Ceiling combo (in place of old output gain fader)
            int ceilingX = faderStartX + (faderW + faderGap) * 2;
            mCeilingLabel.setBounds(ceilingX, ioModule.getY() + 8, faderW + 20, 18);
            mCeilingCombo.setBounds(ceilingX, ioModule.getY() + 30, faderW + 20, 28);

            // Right section: GR Meter + Settings
            int rightX = ioModule.getX() + ioModule.getWidth() - 280 - padding;
            mGRMeter.setBounds(rightX, ioModule.getY() + 20, 280, 48);

            // OS, SR combos, and True Peak toggle
            mOSCombo.setBounds(rightX, ioModule.getY() + 76, 110, 28);
            mSRCombo.setBounds(rightX + 120, ioModule.getY() + 76, 120, 28);
            mTruePeakButton.setBounds(rightX, ioModule.getY() + 110, 82, 22);
            mTruePeakHint.setBounds(rightX + 84, ioModule.getY() + 112, 180, 16);
        }

        content.removeFromTop(moduleGap);

        // Module 2: Processing Modules - SMALLER
        int pmX = content.getX();
        int pmY = content.getY();
        int pmW = content.getWidth();
        int pmH = bottomH;
        int moduleW = (pmW - moduleGap * 2) / 3;

        // Sub-module: Saturation
        mSatGroup.setBounds(pmX, pmY, moduleW, pmH);
        {
            int gx = pmX + padding;
            int gy = pmY + 28;
            mSatOnButton.setBounds(gx, gy, 55, 24);

            int sliderY = gy + 38;
            int sliderH = pmH - 38 - 28 - padding;
            int knobW = (moduleW - padding * 2 - 10) / 2;

            mOddEvenLabel.setBounds(gx, sliderY - 5, knobW, 16);
            mOddEvenSlider.setBounds(gx, sliderY, knobW, sliderH);

            mDriveLabel.setBounds(gx + knobW + 10, sliderY - 5, knobW, 16);
            mDriveSlider.setBounds(gx + knobW + 10, sliderY, knobW, sliderH);
        }

        // Sub-module: Soft Clip
        int clipX = pmX + moduleW + moduleGap;
        mClipGroup.setBounds(clipX, pmY, moduleW, pmH);
        {
            int cx = clipX + padding;
            int cy = pmY + 28;
            mClipOnButton.setBounds(cx + (moduleW - padding * 2 - 55) / 2, cy + 40, 55, 24);
            mRatioCombo.setBounds(cx + (moduleW - padding * 2 - 110) / 2, cy + 80, 110, 28);
            // Clipper LED indicator
            int ledSize = 24;
            mClipperLed.setBounds(cx + moduleW - padding * 2 - ledSize, cy + 42, ledSize, ledSize);
        }

        // Sub-module: Limiter
        int limX = pmX + (moduleW + moduleGap) * 2;
        mLimiterGroup.setBounds(limX, pmY, moduleW, pmH);
        {
            int lx = limX + padding;
            int ly = pmY + 28;
            int lSliderH = pmH - 28 - 28 - padding - 30;
            int lKnobW = (moduleW - padding * 2 - 20) / 3;

            mReleaseLabel.setBounds(lx, ly, lKnobW, 16);
            mReleaseSlider.setBounds(lx, ly + 18, lKnobW, lSliderH);
            // Percentage label overlays the slider's text box when adaptive is on
            mReleasePctLabel.setBounds(lx, ly + 18 + lSliderH - 16, lKnobW, 16);

            int col2 = lx + lKnobW + 10;
            mLookAheadLabel.setBounds(col2, ly, lKnobW, 16);
            mLookAheadSlider.setBounds(col2, ly + 18, lKnobW, lSliderH);

            int col3 = col2 + lKnobW + 10;
            mStereoLinkLabel.setBounds(col3, ly, lKnobW, 16);
            mStereoLinkSlider.setBounds(col3, ly + 18, lKnobW, lSliderH);

            mAutoReleaseButton.setBounds(lx, ly + 18 + lSliderH + 6, 72, 22);
            mAutoReleaseHint.setBounds(lx + 74, ly + 18 + lSliderH + 8, 140, 16);
        }

        // Hide Analysis page components
        mSpectrumAnalyzer.setVisible(false);
        mGRHistoryDisplay.setVisible(false);
        mOutputMeter.setVisible(false);
        mOutputDbLabel.setVisible(false);
        mOutputRmsLabel.setVisible(false);
        mTruePeakLabel.setVisible(false);
        mLufsDisplay.setVisible(false);

        mTiltTitle.setVisible(false);
        mTiltOnButton.setVisible(false);
        mTiltAmountSlider.setVisible(false);
        mTiltAmountLabel.setVisible(false);
        for (int i = 0; i < 4; ++i) {
            mTiltMuddyButtons[i].setVisible(false);
            mTiltMuddyFreqLabels[i].setVisible(false);
        }
        mTiltCleaningLabel.setVisible(false);

    } else {
        // === Analysis Page Layout - More Spacious ===

        // Show Analysis page components
        mSpectrumAnalyzer.setVisible(true);
        mGRHistoryDisplay.setVisible(true);
        mOutputMeter.setVisible(false);
        mOutputDbLabel.setVisible(true);
        mOutputRmsLabel.setVisible(true);
        mTruePeakLabel.setVisible(true);
        mLufsDisplay.setVisible(true);

        // Hide Controls page components (groups, sliders)
        mSatGroup.setVisible(false);
        mClipGroup.setVisible(false);
        mLimiterGroup.setVisible(false);
        mInputGainSlider.setVisible(false);
        mInputGainLabel.setVisible(false);
        mClipperLed.setVisible(false);
        mCeilingCombo.setVisible(false);
        mCeilingLabel.setVisible(false);
        mMixSlider.setVisible(false);
        mMixLabel.setVisible(false);
        mOddEvenSlider.setVisible(false);
        mOddEvenLabel.setVisible(false);
        mDriveSlider.setVisible(false);
        mDriveLabel.setVisible(false);
        mReleaseSlider.setVisible(false);
        mReleaseLabel.setVisible(false);
        mLookAheadSlider.setVisible(false);
        mLookAheadLabel.setVisible(false);
        mAutoReleaseButton.setVisible(false);
        mAutoReleaseHint.setVisible(false);
        mSatOnButton.setVisible(false);
        mClipOnButton.setVisible(false);
        mOSCombo.setVisible(false);
        mOSLabel.setVisible(false);
        mSRCombo.setVisible(false);
        mTruePeakButton.setVisible(false);
        mTruePeakHint.setVisible(false);
        mSRLabel.setVisible(false);
        mRatioCombo.setVisible(false);
        mRatioLabel.setVisible(false);
        mStereoLinkSlider.setVisible(false);
        mStereoLinkLabel.setVisible(false);
        mGRMeter.setVisible(false);

        // === Analysis Page - Modular Layout ===
        int moduleGap = 12;

        // Module 1: Spectrum Analyzer
        auto spectrumModule = content.removeFromTop(static_cast<int>(content.getHeight() * 0.38f));
        mSpectrumAnalyzer.setBounds(spectrumModule.reduced(4));

        content.removeFromTop(moduleGap);

        // Module 2: Tilt EQ controls
        auto tiltModule = content.removeFromTop(static_cast<int>(content.getHeight() * 0.24f));
        {
            int tx = tiltModule.getX() + 8;
            int ty = tiltModule.getY() + 4;
            int tw = tiltModule.getWidth() - 16;
            int rowH = 22; // row spacing (equal for all three rows)

            // Row 1: Tico Magic
            mTiltTitle.setBounds(tx, ty, 70, 18);
            mTiltOnButton.setBounds(tx + 76, ty, 40, 18);

            // Row 2: Kirakira (equal spacing)
            int row2 = ty + rowH + 4;
            mTiltAmountLabel.setBounds(tx, row2, 70, 18);
            mTiltAmountSlider.setBounds(tx + 76, row2, tw - 140, 22);

            // Row 3: PikaPika (equal spacing)
            int row3 = row2 + rowH + 4;
            mTiltCleaningLabel.setBounds(tx, row3, 70, 18);
            int cbX = tx + 76;
            for (int i = 0; i < 4; ++i) {
                mTiltMuddyButtons[i].setBounds(cbX + i * 68, row3, 64, 18);
                mTiltMuddyFreqLabels[i].setBounds(cbX + i * 68, row3 + 18, 64, 12);
            }
        }

        content.removeFromTop(moduleGap);

        // Module 3: GR History
        auto grModule = content.removeFromTop(static_cast<int>(content.getHeight() * 0.55f));
        mGRHistoryDisplay.setBounds(grModule.reduced(4));

        content.removeFromTop(moduleGap);

        // Module 4: Meters
        auto metersModule = content;
        int meterW = 80;

        // Output labels (left column)
        int my = metersModule.getY();
        mOutputDbLabel.setBounds(metersModule.getX() + 4, my + 4, meterW, 16);
        mOutputRmsLabel.setBounds(metersModule.getX() + 4, my + 22, meterW, 16);
        mTruePeakLabel.setBounds(metersModule.getX() + 4, my + 40, meterW, 16);

        // LUFS display (fills remaining width)
        mLufsDisplay.setBounds(metersModule.getX() + meterW + 4, my + 4,
                               metersModule.getWidth() - meterW - 8, metersModule.getHeight() - 8);
    }
}

void TicoLimiterEditor::updatePageVisibility() {
    bool isControls = (mCurrentPage == Page::Controls);

    // Controls page - Groups first, then their children
    mSatGroup.setVisible(isControls);
    mSatOnButton.setVisible(isControls);
    mOddEvenSlider.setVisible(isControls);
    mOddEvenLabel.setVisible(isControls);
    mDriveSlider.setVisible(isControls);
    mDriveLabel.setVisible(isControls);

    mClipGroup.setVisible(isControls);
    mClipOnButton.setVisible(isControls);
    mRatioCombo.setVisible(isControls);
    mStereoLinkSlider.setVisible(isControls);
    mStereoLinkLabel.setVisible(isControls);

    mLimiterGroup.setVisible(isControls);
    mReleaseSlider.setVisible(isControls);
    mReleaseLabel.setVisible(isControls);
    mReleasePctLabel.setVisible(false); // always hidden, shown dynamically in timer
    mLookAheadSlider.setVisible(isControls);
    mLookAheadLabel.setVisible(isControls);
    mAutoReleaseButton.setVisible(isControls);
    mAutoReleaseHint.setVisible(isControls);

    mInputGainSlider.setVisible(isControls);
    mInputGainLabel.setVisible(isControls);
    mClipperLed.setVisible(isControls);
    mMixSlider.setVisible(isControls);
    mMixLabel.setVisible(isControls);
    mCeilingCombo.setVisible(isControls);
    mCeilingLabel.setVisible(isControls);

    mGRMeter.setVisible(isControls);
    mOSCombo.setVisible(isControls);
    mSRCombo.setVisible(isControls);
    mTruePeakButton.setVisible(isControls);
    mTruePeakHint.setVisible(isControls);

    // Analysis page
    bool isAnalysis = (mCurrentPage == Page::Analysis);
    mSpectrumAnalyzer.setVisible(isAnalysis);
    mGRHistoryDisplay.setVisible(isAnalysis);
    mOutputMeter.setVisible(isAnalysis);
    mOutputDbLabel.setVisible(isAnalysis);
    mOutputRmsLabel.setVisible(isAnalysis);
    mTruePeakLabel.setVisible(isAnalysis);
    mLufsDisplay.setVisible(isAnalysis);

    mTiltTitle.setVisible(isAnalysis);
    mTiltOnButton.setVisible(isAnalysis);
    mTiltAmountSlider.setVisible(isAnalysis);
    mTiltAmountLabel.setVisible(isAnalysis);
    for (int i = 0; i < 4; ++i) {
        mTiltMuddyButtons[i].setVisible(isAnalysis);
        mTiltMuddyFreqLabels[i].setVisible(isAnalysis);
    }
    mTiltCleaningLabel.setVisible(isAnalysis);
}

juce::Rectangle<int> TicoLimiterEditor::getTabArea() const {
    auto b = getLocalBounds().reduced(22);
    b.removeFromTop(64 + 12); // title block + gap
    return b.removeFromTop(40);
}

juce::Rectangle<int> TicoLimiterEditor::getContentArea() const {
    auto b = getLocalBounds().reduced(22);
    b.removeFromTop(64 + 12);
    b.removeFromTop(40);
    b.removeFromTop(12);
    return b;
}

void TicoLimiterEditor::drawTabBar(juce::Graphics& g) {
    auto tabArea = getTabArea();
    auto cornerSize = 10.0f;

    // Tab bar background - soft pink
    g.setColour(KawaiiLookAndFeel::Colors::panelBg.withAlpha(0.15f));
    g.fillRoundedRectangle(tabArea.toFloat(), cornerSize);

    // Active tab highlight - sakura pink
    auto activeTab = (mCurrentPage == Page::Controls)
        ? mControlsTabButton.getBounds().toFloat()
        : mAnalysisTabButton.getBounds().toFloat();

    g.setColour(KawaiiLookAndFeel::Colors::sakuraPink.withAlpha(0.25f));
    g.fillRoundedRectangle(activeTab.expanded(2.0f), cornerSize);

    // Tab separator
    g.setColour(KawaiiLookAndFeel::Colors::borderLight);
    float sepX = mControlsTabButton.getRight() + 6.0f;
    g.drawLine(sepX, tabArea.getY() + 5.0f, sepX, tabArea.getBottom() - 5.0f, 1.0f);
}

void TicoLimiterEditor::drawDeco(juce::Graphics& g) {
    // Subtle background hearts
    struct Heart { float x, y, size; juce::Colour colour; };
    static const Heart hearts[] = {
        {70.0f,  180.0f, 8.0f,  KawaiiLookAndFeel::Colors::sakuraLight},
        {920.0f, 420.0f, 6.0f,  KawaiiLookAndFeel::Colors::sakuraPink},
        {150.0f, 580.0f, 7.0f,  KawaiiLookAndFeel::Colors::lavender},
        {860.0f, 120.0f, 5.0f,  KawaiiLookAndFeel::Colors::sakuraLight},
    };
    for (auto& h : hearts) {
        g.setColour(h.colour.withAlpha(0.12f));
        juce::Path heart;
        float s = h.size;
        float cx = h.x, cy = h.y;
        heart.startNewSubPath(cx, cy + s * 0.3f);
        heart.cubicTo(cx, cy, cx - s * 0.5f, cy, cx - s * 0.5f, cy + s * 0.15f);
        heart.cubicTo(cx - s * 0.5f, cy + s * 0.4f, cx, cy + s * 0.6f, cx, cy + s * 0.8f);
        heart.cubicTo(cx, cy + s * 0.6f, cx + s * 0.5f, cy + s * 0.4f, cx + s * 0.5f, cy + s * 0.15f);
        heart.cubicTo(cx + s * 0.5f, cy, cx, cy, cx, cy + s * 0.3f);
        g.fillPath(heart);
    }
}

void TicoLimiterEditor::drawAnimatedStars(juce::Graphics& g, float phase) {
    // Kawaii sparkle stars
    struct Star { float x, y, size, speed; juce::Colour colour; };
    static const Star stars[] = {
        {45, 65, 6, 1.0f, KawaiiLookAndFeel::Colors::sakuraPink},
        {880, 45, 5, 1.2f, KawaiiLookAndFeel::Colors::skyBlue},
        {65, 550, 4, 0.9f, KawaiiLookAndFeel::Colors::lavender},
        {850, 560, 6, 1.1f, KawaiiLookAndFeel::Colors::sakuraPink},
        {480, 28, 5, 1.4f, KawaiiLookAndFeel::Colors::skyBlue},
        {240, 580, 4, 0.8f, KawaiiLookAndFeel::Colors::sakuraPink},
        {700, 590, 5, 1.3f, KawaiiLookAndFeel::Colors::lavender},
        {180, 92, 4, 1.5f, KawaiiLookAndFeel::Colors::skyBlue},
        {760, 85, 5, 0.7f, KawaiiLookAndFeel::Colors::sakuraPink},
    };

    float gi = mLookAndFeel.glowIntensity;
    for (auto& st : stars) {
        float alpha = 0.1f + 0.08f * std::sin(phase * st.speed + st.x * 0.1f) + gi * 0.06f;
        g.setColour(st.colour.withAlpha(alpha));

        // Draw 4-point star
        juce::Path star;
        star.addStar(juce::Point<float>(st.x, st.y), 4, st.size * 0.25f, st.size, phase * st.speed * 0.3f);
        g.fillPath(star);

        // Soft glow
        if (gi > 0.1f) {
            g.setColour(st.colour.withAlpha(alpha * gi * 0.3f));
            g.fillEllipse(st.x - st.size * 1.5f, st.y - st.size * 1.5f, st.size * 3.0f, st.size * 3.0f);
        }

        // Center sparkle
        g.setColour(juce::Colours::white.withAlpha(alpha * 0.5f));
        g.fillEllipse(st.x - 1.5f, st.y - 1.5f, 3.0f, 3.0f);
    }
}

void TicoLimiterEditor::refreshPresetCombo() {
    mPresetCombo.clear(juce::dontSendNotification);
    auto names = mPresetManager->getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        mPresetCombo.addItem(names[i], i + 1);

    auto current = mPresetManager->getCurrentPreset();
    int idx = mPresetManager->getPresetIndex(current);
    mPresetCombo.setSelectedItemIndex(idx >= 0 ? idx : 0, juce::dontSendNotification);
}
