#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <algorithm>

// Graphical LUFS meter with peak hold
class LufsDisplay : public juce::Component, private juce::Timer {
public:
    LufsDisplay() {
        startTimerHz(30);
    }
    ~LufsDisplay() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto cornerSize = 8.0f;

        // Background - pink tint
        g.setColour(juce::Colour(0x10FF6B9D));
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(juce::Colour(0x20FF6B9D));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

        // Title
        g.setColour(juce::Colour(0xFFE8658A));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::bold)));
        g.drawText("LUFS", bounds.removeFromTop(14.0f).reduced(4.0f, 0.0f), juce::Justification::centredLeft);

        auto plotArea = bounds.reduced(8.0f, 4.0f);

        // Draw three LUFS meters (M, S, I)
        float meterHeight = plotArea.getHeight() / 3.0f - 4.0f;
        float meterWidth = plotArea.getWidth() - 50.0f;

        drawLufsMeter(g, plotArea.getX(), plotArea.getY(), meterWidth, meterHeight,
                      mCurrentMomentary, mPeakMomentary, "M", juce::Colour(0xFF4A90D9));
        drawLufsMeter(g, plotArea.getX(), plotArea.getY() + meterHeight + 4, meterWidth, meterHeight,
                      mCurrentShortTerm, mPeakShortTerm, "S", juce::Colour(0xFFFF6B9D));
        drawLufsMeter(g, plotArea.getX(), plotArea.getY() + (meterHeight + 4) * 2, meterWidth, meterHeight,
                      mCurrentIntegrated, mPeakIntegrated, "I", juce::Colour(0xFFE8658A));
    }

    void setMomentary(float lufs) {
        mCurrentMomentary = lufs;
        if (lufs > mPeakMomentary) mPeakMomentary = lufs;
    }

    void setShortTerm(float lufs) {
        mCurrentShortTerm = lufs;
        if (lufs > mPeakShortTerm) mPeakShortTerm = lufs;
    }

    void setIntegrated(float lufs) {
        mCurrentIntegrated = lufs;
        if (lufs > mPeakIntegrated) mPeakIntegrated = lufs;
    }

    void resetPeaks() {
        mPeakMomentary = -70.0f;
        mPeakShortTerm = -70.0f;
        mPeakIntegrated = -70.0f;
    }

private:
    void timerCallback() override {
        repaint();
    }

    void drawLufsMeter(juce::Graphics& g, float x, float y, float width, float height,
                       float current, float peak, const juce::String& label, juce::Colour color) {
        // Label
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::bold)));
        g.drawText(label, x, y, 12.0f, height, juce::Justification::centredLeft);

        // Meter background
        float meterX = x + 14.0f;
        float meterWidth = width - 14.0f;
        g.setColour(juce::Colour(0x15000000));
        g.fillRoundedRectangle(meterX, y, meterWidth, height, 3.0f);

        // Current value bar
        float normalizedCurrent = std::clamp((current + 70.0f) / 70.0f, 0.0f, 1.0f);
        float barWidth = meterWidth * normalizedCurrent;

        // Gradient fill
        juce::ColourGradient gradient(color.withAlpha(0.8f), meterX, y,
                                       color.withAlpha(0.4f), meterX + barWidth, y, false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(meterX, y, barWidth, height, 3.0f);

        // Peak indicator
        float normalizedPeak = std::clamp((peak + 70.0f) / 70.0f, 0.0f, 1.0f);
        float peakX = meterX + meterWidth * normalizedPeak;
        g.setColour(color);
        g.fillRect(peakX - 1.0f, y, 2.0f, height);

        // Value text
        g.setColour(juce::Colour(0xFF333333));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 8.0f, juce::Font::plain)));
        juce::String valueText = current <= -69.0f ? "---" : juce::String(current, 1);
        g.drawText(valueText, meterX + meterWidth + 4.0f, y, 30.0f, height, juce::Justification::centredLeft);

        // Peak value
        g.setColour(color.withAlpha(0.7f));
        juce::String peakText = peak <= -69.0f ? "---" : juce::String(peak, 1);
        g.drawText(peakText, meterX + meterWidth + 34.0f, y, 30.0f, height, juce::Justification::centredLeft);
    }

    float mCurrentMomentary = -70.0f;
    float mCurrentShortTerm = -70.0f;
    float mCurrentIntegrated = -70.0f;

    float mPeakMomentary = -70.0f;
    float mPeakShortTerm = -70.0f;
    float mPeakIntegrated = -70.0f;
};
