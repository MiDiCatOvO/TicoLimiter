#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <algorithm>
#include <array>

class SpectrumAnalyzer : public juce::Component, private juce::Timer {
public:
    SpectrumAnalyzer() {
        startTimerHz(30);
        mInputSpectrum.fill(-80.0f);
        mOutputSpectrum.fill(-80.0f);
    }
    ~SpectrumAnalyzer() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto cornerSize = 10.0f;

        // Panel background - kawaii style
        g.setColour(juce::Colour(0x15FF8FAB));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(juce::Colour(0x30FF8FAB));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

        // Title
        auto titleArea = bounds.removeFromTop(16.0f).reduced(6.0f, 0.0f);
        g.setColour(juce::Colour(0xFFE8658A));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::bold)));
        g.drawText("Spectrum", titleArea, juce::Justification::centredLeft);

        // Legend
        auto legendArea = bounds.removeFromTop(14.0f).reduced(6.0f, 0.0f);
        float lx = legendArea.getX();
        float ly = legendArea.getY() + 2.0f;
        g.setColour(juce::Colour(0xFF87CEEB));
        g.fillRect(lx, ly, 12.0f, 3.0f);
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::plain)));
        g.drawText("In", lx + 14.0f, ly - 2.0f, 16.0f, 12.0f, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xFFFF8FAB));
        g.fillRect(lx + 36.0f, ly, 12.0f, 3.0f);
        g.setColour(juce::Colour(0xFF666666));
        g.drawText("Out", lx + 50.0f, ly - 2.0f, 20.0f, 12.0f, juce::Justification::centredLeft);

        auto plotArea = bounds.reduced(14.0f, 6.0f);
        plotArea = plotArea.withTrimmedBottom(16.0f);

        drawGrid(g, plotArea);
        drawFreqLabels(g, plotArea);
        drawDbLabels(g, plotArea);

        // Draw output first (behind), then input
        drawSpectrumCurve(g, plotArea, mOutputSpectrumSmooth,
                          juce::Colour(0x30FF8FAB), juce::Colour(0xFFFF8FAB));
        drawSpectrumCurve(g, plotArea, mInputSpectrumSmooth,
                          juce::Colour(0x3087CEEB), juce::Colour(0xFF87CEEB));

        // Tilt EQ curve (green)
        if (mTiltEnabled)
            drawTiltCurve(g, plotArea);
    }

    void setInputSpectrum(const float* data, int numBins) {
        int count = std::min(numBins, kMaxBins);
        for (int i = 0; i < count; ++i) mInputSpectrum[i] = data[i];
    }

    void setOutputSpectrum(const float* data, int numBins) {
        int count = std::min(numBins, kMaxBins);
        for (int i = 0; i < count; ++i) mOutputSpectrum[i] = data[i];
    }

    void setSampleRate(double sampleRate) {
        mSampleRate = sampleRate;
        mNyquist = sampleRate * 0.5;
    }

    // Set tilt EQ curve data (in dB, one value per display bin)
    void setTiltCurve(const std::array<float, 512>& curve, bool enabled) {
        mTiltCurve = curve;
        mTiltEnabled = enabled;
    }

private:
    static constexpr int kMaxBins = 512;

    void timerCallback() override {
        for (int i = 0; i < kMaxBins; ++i) {
            mInputSpectrumSmooth[i]  = mInputSpectrumSmooth[i]  * 0.7f + mInputSpectrum[i]  * 0.3f;
            mOutputSpectrumSmooth[i] = mOutputSpectrumSmooth[i] * 0.7f + mOutputSpectrum[i] * 0.3f;
        }
        repaint();
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area) {
        g.setColour(juce::Colour(0x15000000));
        for (int db = -60; db <= 0; db += 12) {
            float y = area.getY() + (1.0f - (db + 60.0f) / 60.0f) * area.getHeight();
            g.drawLine(area.getX(), y, area.getRight(), y, 0.5f);
        }
        float freqs[] = { 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float freq : freqs) {
            if (freq <= mNyquist) {
                float x = freqToX(freq, area);
                g.drawLine(x, area.getY(), x, area.getBottom(), 0.5f);
            }
        }
    }

    void drawFreqLabels(juce::Graphics& g, juce::Rectangle<float> area) {
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 8.0f, juce::Font::plain)));
        struct FL { float freq; const char* text; };
        FL labels[] = {
            {20,"20"},{50,"50"},{100,"100"},{200,"200"},{500,"500"},
            {1000,"1k"},{2000,"2k"},{5000,"5k"},{10000,"10k"},{20000,"20k"}
        };
        for (auto& fl : labels) {
            if (fl.freq <= mNyquist) {
                float x = freqToX(fl.freq, area);
                g.drawText(fl.text, x - 16.0f, area.getBottom() + 4.0f, 32.0f, 14.0f, juce::Justification::centred);
            }
        }
    }

    void drawDbLabels(juce::Graphics& g, juce::Rectangle<float> area) {
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 8.0f, juce::Font::plain)));
        // Draw dB labels inside the plot area (top right corner)
        for (int db = -60; db <= 0; db += 20) {
            float y = area.getY() + (1.0f - (db + 60.0f) / 60.0f) * area.getHeight();
            g.drawText(juce::String(db), area.getRight() - 30.0f, y - 6.0f, 25.0f, 12.0f, juce::Justification::centredRight);
        }
    }

    void drawSpectrumCurve(juce::Graphics& g, juce::Rectangle<float> area,
                           const std::array<float, kMaxBins>& spectrum,
                           juce::Colour fillColour, juce::Colour lineColour) {
        juce::Path path;
        bool started = false;

        // Log-spaced frequency mapping (matches data stored in PluginProcessor)
        for (int i = 0; i < kMaxBins; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kMaxBins - 1);
            float freq = 20.0f * std::pow(1000.0f, t); // 20 Hz → 20 kHz
            if (freq > 20000.0f) break;

            float x = freqToX(freq, area);
            // Apply equal-loudness compensation and sensitivity boost
            float compensation = getEqualLoudnessCompensation(freq);
            float db = std::clamp(spectrum[i] + compensation + 20.0f, -60.0f, 0.0f);
            float y = area.getY() + (1.0f - (db + 60.0f) / 60.0f) * area.getHeight();
            if (!started) {
                path.startNewSubPath(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
        }

        if (started) {
            juce::Path fillPath(path);
            fillPath.lineTo(freqToX(20000.0f, area), area.getBottom());
            fillPath.lineTo(freqToX(20.0f, area), area.getBottom());
            fillPath.closeSubPath();
            g.setColour(fillColour);
            g.fillPath(fillPath);

            // Line glow
            g.setColour(lineColour.withAlpha(0.3f));
            g.strokePath(path, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));
            g.setColour(lineColour);
            g.strokePath(path, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
        }
    }

    void drawTiltCurve(juce::Graphics& g, juce::Rectangle<float> area) {
        juce::Path path;
        bool started = false;

        for (int i = 0; i < kMaxBins; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kMaxBins - 1);
            float freq = 20.0f * std::pow(1000.0f, t); // 20 Hz → 20 kHz
            if (freq > 20000.0f) break;

            float x = freqToX(freq, area);
            // Map tilt response (dB) to Y position in plot area
            // Response range: roughly -6 to +6 dB, map to plot area
            float db = std::clamp(mTiltCurve[i], -12.0f, 12.0f);
            float norm = (db + 12.0f) / 24.0f; // 0 to 1
            float y = area.getY() + (1.0f - norm) * area.getHeight();

            if (!started) {
                path.startNewSubPath(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
        }

        if (started) {
            g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.3f));
            g.strokePath(path, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));
            g.setColour(juce::Colour(0xFF00FF88));
            g.strokePath(path, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
        }
    }

    float freqToX(float freq, juce::Rectangle<float> area) const {
        float norm = (std::log10(std::clamp(freq, 20.0f, 20000.0f)) - std::log10(20.0f))
                   / (std::log10(20000.0f) - std::log10(20.0f));
        return area.getX() + norm * area.getWidth();
    }

    // Equal-loudness compensation (ISO 226 approximation)
    // Returns dB gain to apply so that equal perceived loudness appears at equal height
    float getEqualLoudnessCompensation(float freq) const {
        // Approximate ISO 226 equal-loudness contour inverse
        // Based on 80 phon contour, normalized to 1 kHz = 0 dB
        if (freq < 20.0f) return -30.0f;
        if (freq > 20000.0f) return -10.0f;

        // Use piecewise linear approximation
        // Low frequencies: steep rolloff
        if (freq < 100.0f) {
            return -20.0f + 10.0f * (freq - 20.0f) / 80.0f;  // -20 dB at 20 Hz, -10 dB at 100 Hz
        }
        if (freq < 200.0f) {
            return -10.0f + 5.0f * (freq - 100.0f) / 100.0f;  // -10 dB at 100 Hz, -5 dB at 200 Hz
        }
        if (freq < 500.0f) {
            return -5.0f + 5.0f * (freq - 200.0f) / 300.0f;   // -5 dB at 200 Hz, 0 dB at 500 Hz
        }
        if (freq < 1000.0f) {
            return 0.0f;  // 0 dB at 500-1000 Hz
        }
        if (freq < 2000.0f) {
            return 5.0f * (freq - 1000.0f) / 1000.0f;  // 0 dB at 1 kHz, +5 dB at 2 kHz
        }
        if (freq < 4000.0f) {
            return 5.0f + 5.0f * (freq - 2000.0f) / 2000.0f;  // +5 dB at 2 kHz, +10 dB at 4 kHz
        }
        if (freq < 6000.0f) {
            return 10.0f + 3.0f * (freq - 4000.0f) / 2000.0f;  // +10 dB at 4 kHz, +13 dB at 6 kHz
        }
        if (freq < 10000.0f) {
            return 13.0f;  // +13 dB at 6-10 kHz
        }
        if (freq < 15000.0f) {
            return 13.0f - 5.0f * (freq - 10000.0f) / 5000.0f;  // +13 dB at 10 kHz, +8 dB at 15 kHz
        }
        // 15-20 kHz: slight rolloff
        return 8.0f - 5.0f * (freq - 15000.0f) / 5000.0f;  // +8 dB at 15 kHz, +3 dB at 20 kHz
    }

    std::array<float, kMaxBins> mInputSpectrum{};
    std::array<float, kMaxBins> mOutputSpectrum{};
    std::array<float, kMaxBins> mInputSpectrumSmooth{};
    std::array<float, kMaxBins> mOutputSpectrumSmooth{};
    std::array<float, 512> mTiltCurve{};
    bool mTiltEnabled = false;
    double mSampleRate = 44100.0;
    double mNyquist = 22050.0;
};
