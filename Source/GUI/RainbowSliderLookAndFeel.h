#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Custom LookAndFeel for rainbow-colored horizontal slider
class RainbowSliderLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height));

        // Track area (centered vertically)
        float trackH = 6.0f;
        float trackY = bounds.getCentreY() - trackH * 0.5f;
        auto trackBounds = juce::Rectangle<float>(bounds.getX() + 8.0f, trackY,
                                                   bounds.getWidth() - 16.0f, trackH);

        // Draw rainbow gradient track (background)
        g.setColour(juce::Colour(0x20000000));
        g.fillRoundedRectangle(trackBounds, 3.0f);

        // Rainbow gradient: red → orange → yellow → green → cyan → blue → violet
        juce::ColourGradient rainbow(
            juce::Colour(0xFFFF4444), trackBounds.getX(), 0.0f,
            juce::Colour(0xFF8844FF), trackBounds.getRight(), 0.0f, false
        );
        rainbow.addColour(0.14, juce::Colour(0xFFFF8800)); // orange
        rainbow.addColour(0.28, juce::Colour(0xFFFFDD00)); // yellow
        rainbow.addColour(0.42, juce::Colour(0xFF44DD44)); // green
        rainbow.addColour(0.57, juce::Colour(0xFF00CCCC)); // cyan
        rainbow.addColour(0.71, juce::Colour(0xFF4488FF)); // blue
        rainbow.addColour(0.85, juce::Colour(0xFF6644DD)); // indigo

        // Fill the track with rainbow
        g.setGradientFill(rainbow);
        g.fillRoundedRectangle(trackBounds, 3.0f);

        // Glow effect
        g.setColour(juce::Colour(0x15FFFFFF));
        g.fillRoundedRectangle(trackBounds.expanded(0.0f, 1.0f), 4.0f);

        // Thumb (circular)
        float thumbR = 9.0f;
        float thumbX = sliderPos;
        float thumbY = bounds.getCentreY();
        auto thumbBounds = juce::Rectangle<float>(thumbX - thumbR, thumbY - thumbR,
                                                   thumbR * 2.0f, thumbR * 2.0f);

        // Thumb glow
        g.setColour(juce::Colour(0x40FFFFFF));
        g.fillEllipse(thumbBounds.expanded(2.0f));

        // Thumb body - white with subtle border
        g.setColour(juce::Colours::white);
        g.fillEllipse(thumbBounds);
        g.setColour(juce::Colour(0x60000000));
        g.drawEllipse(thumbBounds, 1.0f);

        // Center dot
        g.setColour(juce::Colour(0xFFE8658A));
        g.fillEllipse(thumbX - 2.5f, thumbY - 2.5f, 5.0f, 5.0f);
    }
};
