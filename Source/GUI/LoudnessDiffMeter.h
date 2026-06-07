#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <algorithm>

// Simple vertical meter showing loudness headroom
// Full = no limiting, can push input more
// Empty = heavy limiting, pushing input won't increase output
class LoudnessDiffMeter : public juce::Component, private juce::Timer {
public:
    LoudnessDiffMeter() { startTimerHz(20); }
    ~LoudnessDiffMeter() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(juce::Colour(0x15FF8FAB));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Label at top
        g.setColour(juce::Colour(0xFF999999));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 7.0f, juce::Font::bold)));
        g.drawText("HEAD", bounds.removeFromTop(10.0f), juce::Justification::centred);

        auto meterArea = bounds.reduced(3.0f, 2.0f);

        // Track background
        g.setColour(juce::Colour(0x0AFFFFFF));
        g.fillRoundedRectangle(meterArea, 2.0f);

        // Fill level (0 = no headroom, 1 = full headroom)
        float fill = std::clamp(mDisplayValue, 0.0f, 1.0f);
        float fillH = meterArea.getHeight() * fill;
        auto fillBounds = juce::Rectangle<float>(
            meterArea.getX(),
            meterArea.getBottom() - fillH,
            meterArea.getWidth(),
            fillH
        );

        // Gradient: red (bottom, heavy limiting) -> yellow -> green (top, headroom)
        juce::ColourGradient gradient(
            juce::Colour(0xFFFF6B6B), meterArea.getX(), meterArea.getBottom(),
            juce::Colour(0xFF7DFFAA), meterArea.getX(), meterArea.getY(), false
        );
        gradient.addColour(0.4, juce::Colour(0xFFFFD700));
        gradient.addColour(0.7, juce::Colour(0xFF7DFFA0));
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fillBounds, 2.0f);

        // Glow
        if (fill > 0.05f) {
            g.setColour(juce::Colour(0xFF7DFFAA).withAlpha(0.12f));
            g.fillRoundedRectangle(fillBounds.expanded(1.0f), 3.0f);
        }
    }

    // Set headroom: 0.0 = no headroom (heavy GR), 1.0 = full headroom (no GR)
    void setHeadroom(float headroom) { mTargetValue = headroom; }

private:
    void timerCallback() override {
        mDisplayValue += 0.2f * (mTargetValue - mDisplayValue);
        repaint();
    }

    float mTargetValue = 1.0f;
    float mDisplayValue = 1.0f;
};
