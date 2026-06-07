#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <algorithm>

// Neon-style meter component with glow effects
class MeterComponent : public juce::Component, private juce::Timer {
public:
    enum class MeterType { GainReduction, Level };

    MeterComponent(MeterType type = MeterType::GainReduction)
        : mType(type)
    {
        startTimerHz(30);
    }

    ~MeterComponent() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto cornerSize = 8.0f;

        // Background panel - kawaii style
        g.setColour(juce::Colour(0x15FF8FAB));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(juce::Colour(0x30FF8FAB));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

        if (mType == MeterType::GainReduction) {
            paintGainReduction(g, bounds);
        } else {
            paintLevel(g, bounds);
        }

        // Label
        g.setColour(juce::Colour(0xFF555555));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::bold)));
        g.drawText(mLabel, bounds.removeFromBottom(12.0f).reduced(2.0f, 0.0f), juce::Justification::centred);
    }

    void setGainReductionDb(float grDb) { mTargetGR = std::min(grDb, 0.0f); }
    void setLevelDb(float levelDb) { mTargetLevel = levelDb; }
    void setLabel(const juce::String& label) { mLabel = label; }

private:
    void timerCallback() override {
        mDisplayGR = mDisplayGR + 0.3f * (mTargetGR - mDisplayGR);
        mDisplayLevel = mDisplayLevel + 0.3f * (mTargetLevel - mDisplayLevel);
        repaint();
    }

    void paintGainReduction(juce::Graphics& g, juce::Rectangle<float> bounds) {
        auto meterBounds = bounds.reduced(6.0f, 20.0f);
        auto meterWidth = meterBounds.getWidth();
        auto meterHeight = meterBounds.getHeight();

        float normalizedGR = std::clamp(-mDisplayGR / 12.0f, 0.0f, 1.0f);

        int numSegments = 24;
        float segmentWidth = meterWidth / static_cast<float>(numSegments);
        float gap = 1.5f;

        for (int i = 0; i < numSegments; ++i) {
            float segmentPos = static_cast<float>(i) / static_cast<float>(numSegments);
            // Right-to-left: segments fill from the right as GR increases
            int ri = numSegments - 1 - i;
            auto segBounds = juce::Rectangle<float>(
                meterBounds.getX() + static_cast<float>(ri) * segmentWidth + gap * 0.5f,
                meterBounds.getY(),
                segmentWidth - gap,
                meterHeight
            );

            juce::Colour segColour;
            if (segmentPos > 0.75f) {
                segColour = juce::Colour(0xFFFFB0C4); // pink: -9 to -12
            } else if (segmentPos > 0.5f) {
                segColour = juce::Colour(0xFFFFB347); // orange: -6 to -9
            } else if (segmentPos > 0.25f) {
                segColour = juce::Colour(0xFFFFF080); // yellow: -3 to -6
            } else {
                segColour = juce::Colour(0xFFA0FFB0); // green: 0 to -3
            }

            if (segmentPos <= normalizedGR) {
                g.setColour(segColour.withAlpha(0.15f));
                g.fillRoundedRectangle(segBounds.expanded(1.0f), 3.0f);
                g.setColour(segColour.withAlpha(0.85f));
                g.fillRoundedRectangle(segBounds, 2.0f);
            } else {
                g.setColour(segColour.withAlpha(0.1f));
                g.fillRoundedRectangle(segBounds, 2.0f);
            }
        }

        // Scale labels below meter (right-to-left: 0 on right, -12 on left)
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 7.0f, juce::Font::plain)));
        for (int db = 0; db >= -12; db -= 3) {
            float x = meterBounds.getRight() - (static_cast<float>(-db) / 12.0f) * meterWidth;
            g.drawText(juce::String(db), x - 8.0f, meterBounds.getBottom() + 1.0f, 16.0f, 10.0f, juce::Justification::centred);
        }

        // Heart indicator (tracks from right to left)
        if (normalizedGR > 0.01f) {
            float heartX = meterBounds.getRight() - normalizedGR * meterWidth;
            float heartY = meterBounds.getY() - 6.0f;
            g.setColour(juce::Colour(0xFFFF69B4).withAlpha(0.3f));
            g.fillEllipse(heartX - 6.0f, heartY - 2.0f, 12.0f, 12.0f);
            drawHeart(g, heartX, heartY, 7.0f, juce::Colour(0xFFFF69B4));
        }
    }

    void paintLevel(juce::Graphics& g, juce::Rectangle<float> bounds) {
        auto meterBounds = bounds.reduced(6.0f, 20.0f);
        auto meterWidth = meterBounds.getWidth();
        auto meterHeight = meterBounds.getHeight();

        float normalizedLevel = std::clamp((mDisplayLevel + 60.0f) / 60.0f, 0.0f, 1.0f);

        // Background track
        g.setColour(juce::Colour(0x0AFFFFFF));
        g.fillRoundedRectangle(meterBounds, 3.0f);

        // Bar
        auto barBounds = juce::Rectangle<float>(
            meterBounds.getX(), meterBounds.getY(),
            meterWidth * normalizedLevel, meterHeight
        );

        // Gradient: green -> cyan -> pink
        juce::ColourGradient gradient(
            juce::Colour(0xFF7DFFAA), meterBounds.getX(), 0.0f,
            juce::Colour(0xFFFF6B9D), meterBounds.getRight(), 0.0f, false
        );
        gradient.addColour(0.5, juce::Colour(0xFF00D4FF));
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(barBounds, 3.0f);

        // Glow on bar
        if (normalizedLevel > 0.01f) {
            g.setColour(juce::Colour(0xFF00D4FF).withAlpha(0.15f));
            g.fillRoundedRectangle(barBounds.expanded(1.0f), 4.0f);
        }

        // Scale labels below meter
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 7.0f, juce::Font::plain)));
        for (int db = -60; db <= 0; db += 12) {
            float x = meterBounds.getX() + (static_cast<float>(db + 60) / 60.0f) * meterWidth;
            g.drawText(juce::String(db), x - 8.0f, meterBounds.getBottom() + 1.0f, 16.0f, 10.0f, juce::Justification::centred);
        }
    }

    void drawHeart(juce::Graphics& g, float cx, float cy, float size, juce::Colour colour) {
        g.setColour(colour);
        juce::Path heart;
        float s = size;
        heart.startNewSubPath(cx, cy + s * 0.3f);
        heart.cubicTo(cx, cy, cx - s * 0.5f, cy, cx - s * 0.5f, cy + s * 0.15f);
        heart.cubicTo(cx - s * 0.5f, cy + s * 0.4f, cx, cy + s * 0.6f, cx, cy + s * 0.8f);
        heart.cubicTo(cx, cy + s * 0.6f, cx + s * 0.5f, cy + s * 0.4f, cx + s * 0.5f, cy + s * 0.15f);
        heart.cubicTo(cx + s * 0.5f, cy, cx, cy, cx, cy + s * 0.3f);
        g.fillPath(heart);
    }

    MeterType mType;
    juce::String mLabel;
    float mTargetGR = 0.0f;
    float mDisplayGR = 0.0f;
    float mTargetLevel = -60.0f;
    float mDisplayLevel = -60.0f;
};
