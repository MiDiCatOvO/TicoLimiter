#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <algorithm>
#include <vector>

class GRHistoryDisplay : public juce::Component, private juce::Timer {
public:
    GRHistoryDisplay() {
        startTimerHz(30);
        mHistory.resize(kHistorySize, 0.0f);
        mClipperHistory.resize(kHistorySize, 0.0f);
    }
    ~GRHistoryDisplay() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto cornerSize = 10.0f;

        // Panel - kawaii style
        g.setColour(juce::Colour(0x15FF8FAB));
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(juce::Colour(0x30FF8FAB));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

        // Title
        g.setColour(juce::Colour(0xFFE8658A));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::bold)));
        g.drawText("Gain Reduction", bounds.removeFromTop(16.0f).reduced(6.0f, 0.0f), juce::Justification::centredLeft);

        // Legend
        auto legendArea = bounds.removeFromTop(12.0f).reduced(6.0f, 0.0f);
        float lx = legendArea.getX();
        float ly = legendArea.getY() + 2.0f;
        g.setColour(juce::Colour(0xFF87CEEB));
        g.fillRect(lx, ly, 10.0f, 3.0f);
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 8.0f, juce::Font::plain)));
        g.drawText("Limiter", lx + 12.0f, ly - 2.0f, 30.0f, 10.0f, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xFFFF8FAB));
        g.fillRect(lx + 48.0f, ly, 10.0f, 3.0f);
        g.setColour(juce::Colour(0xFF666666));
        g.drawText("Clipper", lx + 60.0f, ly - 2.0f, 30.0f, 10.0f, juce::Justification::centredLeft);

        // Reserve space for right-side values
        auto plotArea = bounds.reduced(14.0f, 6.0f);
        plotArea = plotArea.withTrimmedRight(60.0f);

        drawGrid(g, plotArea);
        drawHistory(g, plotArea, mHistory, juce::Colour(0xFF87CEEB));  // Blue for limiter
        drawHistory(g, plotArea, mClipperHistory, juce::Colour(0xFFFF8FAB));  // Pink for clipper
        drawCurrentValue(g, plotArea);
    }

    void setGainReduction(float grDb) { mCurrentGR = std::min(grDb, 0.0f); }
    void setClipperGainReduction(float grDb) { mCurrentClipperGR = std::min(grDb, 0.0f); }

private:
    static constexpr int kHistorySize = 300;
    static constexpr float kMinGR = -12.0f;
    static constexpr float kMaxGR = 0.0f;

    void timerCallback() override {
        mHistory[mWriteIdx] = mCurrentGR;
        mClipperHistory[mWriteIdx] = mCurrentClipperGR;
        mWriteIdx = (mWriteIdx + 1) % kHistorySize;
        repaint();
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area) {
        g.setColour(juce::Colour(0x15000000));
        for (float db = kMaxGR; db >= kMinGR; db -= 3.0f) {
            float y = grToY(db, area);
            g.drawLine(area.getX(), y, area.getRight(), y, 0.5f);
            g.setColour(juce::Colour(0xFF666666));
            g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 8.0f, juce::Font::plain)));
            g.drawText(juce::String(static_cast<int>(db)), area.getX() - 22.0f, y - 5.0f, 18.0f, 10.0f, juce::Justification::centredRight);
            g.setColour(juce::Colour(0x15000000));
        }
    }

    void drawHistory(juce::Graphics& g, juce::Rectangle<float> area, const std::vector<float>& history, juce::Colour color) {
        juce::Path linePath;
        int numPoints = kHistorySize;
        float dx = area.getWidth() / static_cast<float>(numPoints);
        bool started = false;

        for (int i = 0; i < numPoints; ++i) {
            int idx = (mWriteIdx + i) % kHistorySize;
            float gr = std::clamp(history[idx], kMinGR, kMaxGR);
            float x = area.getX() + static_cast<float>(i) * dx;
            float y = grToY(gr, area);
            if (!started) { linePath.startNewSubPath(x, y); started = true; }
            else { linePath.lineTo(x, y); }
        }

        if (started) {
            // Fill
            juce::Path fillPath(linePath);
            fillPath.lineTo(area.getRight(), area.getBottom());
            fillPath.lineTo(area.getX(), area.getBottom());
            fillPath.closeSubPath();

            juce::ColourGradient gradient(
                color.withAlpha(0.0f), area.getX(), area.getY(),
                color.withAlpha(0.25f), area.getX(), area.getBottom(), false);
            gradient.addColour(0.5, color.withAlpha(0.15f));
            g.setGradientFill(gradient);
            g.fillPath(fillPath);

            // Line glow
            g.setColour(color.withAlpha(0.25f));
            g.strokePath(linePath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved));

            // Line
            juce::ColourGradient lineGrad(
                color, area.getX(), area.getY(),
                color.darker(0.2f), area.getX(), area.getBottom(), false);
            g.setGradientFill(lineGrad);
            g.strokePath(linePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
        }
    }

    void drawCurrentValue(juce::Graphics& g, juce::Rectangle<float> area) {
        // Limiter GR indicator - blue
        float gr = std::clamp(mCurrentGR, kMinGR, kMaxGR);
        float y = grToY(gr, area);

        g.setColour(juce::Colour(0xFF87CEEB).withAlpha(0.4f));
        g.fillEllipse(area.getRight() + 2.0f, y - 7.0f, 14.0f, 14.0f);

        g.setColour(juce::Colour(0xFF87CEEB));
        juce::Path tri;
        tri.addTriangle(area.getRight() + 3.0f, y, area.getRight() + 11.0f, y - 5.0f,
                         area.getRight() + 11.0f, y + 5.0f);
        g.fillPath(tri);

        // Limiter value - blue
        g.setColour(juce::Colour(0xFF66CCFF));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::bold)));
        g.drawText(juce::String(gr, 1) + "dB", area.getRight() + 15.0f, y - 8.0f, 50.0f, 16.0f, juce::Justification::centredLeft);

        // Clipper GR indicator - pink
        float clipperGr = std::clamp(mCurrentClipperGR, kMinGR, kMaxGR);
        float clipperY = grToY(clipperGr, area);

        g.setColour(juce::Colour(0xFFFF8FAB).withAlpha(0.4f));
        g.fillEllipse(area.getRight() + 2.0f, clipperY - 7.0f, 14.0f, 14.0f);

        g.setColour(juce::Colour(0xFFFF8FAB));
        juce::Path clipperTri;
        clipperTri.addTriangle(area.getRight() + 3.0f, clipperY, area.getRight() + 11.0f, clipperY - 5.0f,
                         area.getRight() + 11.0f, clipperY + 5.0f);
        g.fillPath(clipperTri);

        // Clipper value - blue
        g.setColour(juce::Colour(0xFF66CCFF));
        g.drawText(juce::String(clipperGr, 1) + "dB", area.getRight() + 15.0f, clipperY - 8.0f, 50.0f, 16.0f, juce::Justification::centredLeft);
    }

    float grToY(float grDb, juce::Rectangle<float> area) const {
        float norm = (grDb - kMaxGR) / (kMinGR - kMaxGR);
        return area.getY() + norm * area.getHeight();
    }

    float mCurrentGR = 0.0f;
    float mCurrentClipperGR = 0.0f;
    std::vector<float> mHistory;
    std::vector<float> mClipperHistory;
    int mWriteIdx = 0;
};
