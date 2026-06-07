#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

class ClipperLED : public juce::Component, private juce::Timer {
public:
    ClipperLED() { startTimerHz(20); }
    ~ClipperLED() override { stopTimer(); }

    // grDb: negative value (e.g. -2.5). Enabled: whether clipper is on.
    void setClipperState(float grDb, bool enabled) {
        mEnabled = enabled;
        mTargetGr = enabled ? std::abs(grDb) : 0.0f;
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        float size = std::min(bounds.getWidth(), bounds.getHeight());
        float radius = size * 0.4f;
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();

        // Background circle (dark)
        g.setColour(juce::Colour(0xFF1A1A1A));
        g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

        // LED colour
        juce::Colour ledColour;
        float glowAlpha = 0.0f;

        if (!mEnabled) {
            // Off: dim grey
            ledColour = juce::Colour(0xFF333333);
        } else if (mDisplayGr < 0.5f) {
            // Green: < 0.5 dB
            ledColour = juce::Colour(0xFF00CC66);
            glowAlpha = 0.3f + 0.2f * (mDisplayGr / 0.5f);
        } else if (mDisplayGr < 1.5f) {
            // Yellow: 0.5-1.5 dB
            ledColour = juce::Colour(0xFFFFCC00);
            glowAlpha = 0.4f + 0.2f * ((mDisplayGr - 0.5f) / 1.0f);
        } else {
            // Red: > 1.5 dB
            ledColour = juce::Colour(0xFFFF3333);
            glowAlpha = 0.6f + 0.2f * std::min((mDisplayGr - 1.5f) / 3.0f, 1.0f);
        }

        // Glow
        if (glowAlpha > 0.01f) {
            g.setColour(ledColour.withAlpha(glowAlpha * 0.4f));
            g.fillEllipse(cx - radius * 1.4f, cy - radius * 1.4f, radius * 2.8f, radius * 2.8f);
        }

        // LED body
        g.setColour(ledColour);
        g.fillEllipse(cx - radius * 0.7f, cy - radius * 0.7f, radius * 1.4f, radius * 1.4f);

        // Specular highlight
        g.setColour(juce::Colours::white.withAlpha(mEnabled ? 0.35f : 0.1f));
        g.fillEllipse(cx - radius * 0.3f, cy - radius * 0.45f, radius * 0.5f, radius * 0.4f);
    }

private:
    void timerCallback() override {
        mDisplayGr += 0.15f * (mTargetGr - mDisplayGr);
        repaint();
    }

    bool mEnabled = false;
    float mTargetGr = 0.0f;
    float mDisplayGr = 0.0f;
};
