#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

// Kawaii Anime Style LookAndFeel - High quality aesthetic design
class KawaiiLookAndFeel : public juce::LookAndFeel_V4 {
public:
    struct Colors {
        // Background - soft pastel
        static inline const juce::Colour background      { 0xFFFFF5F8 }; // soft pink-white
        static inline const juce::Colour backgroundPink   { 0xFFFFF0F5 }; // pink tint
        static inline const juce::Colour panelBg          { 0x15FF8FAB }; // pink panel

        // Main colors - sakura pink theme
        static inline const juce::Colour sakuraPink       { 0xFFFF8FAB }; // main pink
        static inline const juce::Colour sakuraDark       { 0xFFE8658A }; // dark pink
        static inline const juce::Colour sakuraLight      { 0xFFFFB5C5 }; // light pink
        static inline const juce::Colour skyBlue          { 0xFF87CEEB }; // accent blue
        static inline const juce::Colour lavender         { 0xFFB8A9F0 }; // soft purple
        static inline const juce::Colour mintGreen        { 0xFF98D8C8 }; // soft green
        static inline const juce::Colour creamYellow      { 0xFFFFF0C4 }; // warm yellow

        // UI elements
        static inline const juce::Colour faderTrack       { 0x20FF8FAB }; // pink track
        static inline const juce::Colour faderThumb       { 0xFFFF8FAB }; // pink thumb
        static inline const juce::Colour switchOn         { 0xFFFF8FAB }; // pink switch
        static inline const juce::Colour switchOff        { 0xFFCCCCCC }; // gray switch
        static inline const juce::Colour borderLight      { 0x30FF8FAB }; // light border
        static inline const juce::Colour borderMedium     { 0x60FF8FAB }; // medium border

        // Text
        static inline const juce::Colour titleColor       { 0xFFE8658A }; // pink title
        static inline const juce::Colour labelColor       { 0xFF66CCFF }; // BLUE for labels (Tico Power, Mix, etc)
        static inline const juce::Colour valueColor       { 0xFF333333 }; // BLACK for values (0dB, 100%, etc)
        static inline const juce::Colour dimText          { 0xFF999999 }; // dim text

        // Glow effects
        static inline const juce::Colour glowPink         { 0x30FF8FAB };
        static inline const juce::Colour glowBlue         { 0x3087CEEB };
        static inline const juce::Colour glowActive       { 0x50FF8FAB };
    };

    // Dynamic glow intensity
    float glowIntensity = 0.0f;

    KawaiiLookAndFeel() {
        // Slider colors
        setColour(juce::Slider::thumbColourId, Colors::faderThumb);
        setColour(juce::Slider::trackColourId, Colors::faderTrack);
        setColour(juce::Slider::rotarySliderFillColourId, Colors::sakuraPink);
        setColour(juce::Slider::rotarySliderOutlineColourId, Colors::sakuraLight);
        setColour(juce::Slider::textBoxTextColourId, Colors::valueColor);  // Black for values
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00FFFFFF));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00FFFFFF));

        // Button colors
        setColour(juce::ToggleButton::textColourId, Colors::labelColor);
        setColour(juce::TextButton::buttonColourId, Colors::panelBg);
        setColour(juce::TextButton::textColourOnId, Colors::sakuraPink);
        setColour(juce::TextButton::textColourOffId, Colors::dimText);

        // Label colors
        setColour(juce::Label::textColourId, Colors::labelColor);

        // ComboBox colors - black for values
        setColour(juce::ComboBox::textColourId, Colors::valueColor);  // Black for OS, SR values
        setColour(juce::ComboBox::backgroundColourId, Colors::panelBg);
        setColour(juce::ComboBox::outlineColourId, Colors::borderMedium);
    }

    // ========== FADER (Linear Slider) ==========
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height));

        if (style == juce::Slider::LinearVertical) {
            drawFader(g, bounds, sliderPos, slider);
        } else {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                              minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawFader(juce::Graphics& g, juce::Rectangle<float> bounds, float pos, juce::Slider& slider)
    {
        // Minimalist fader design with HIGH CONTRAST top/bottom
        auto trackWidth = 8.0f;
        auto trackX = bounds.getCentreX() - trackWidth * 0.5f;
        auto trackTop = bounds.getY() + 8.0f;
        auto trackBottom = bounds.getBottom() - 20.0f;
        auto trackHeight = trackBottom - trackTop;

        // Calculate normalized position (0-1) from slider value
        double minValue = slider.getMinimum();
        double maxValue = slider.getMaximum();
        double currentValue = slider.getValue();
        float normalizedPos = static_cast<float>((currentValue - minValue) / (maxValue - minValue));

        // Current position (0 = bottom, 1 = top)
        auto currentY = trackBottom - (trackHeight * normalizedPos);

        // Track background (above current position) - DARK GRAY
        g.setColour(juce::Colour(0xFFBBBBBB));
        g.fillRoundedRectangle(trackX, trackTop, trackWidth, currentY - trackTop, 4.0f);

        // Track filled (below current position) - BRIGHT PINK
        g.setColour(juce::Colour(0xFFFF6B9D));
        g.fillRoundedRectangle(trackX, currentY, trackWidth, trackBottom - currentY, 4.0f);

        // Track border
        g.setColour(juce::Colour(0xFF999999));
        g.drawRoundedRectangle(trackX, trackTop, trackWidth, trackHeight, 4.0f, 1.5f);

        // Thumb - horizontal line
        auto thumbWidth = 24.0f;
        auto thumbX = bounds.getCentreX() - thumbWidth * 0.5f;

        // Thumb shadow
        g.setColour(juce::Colour(0x30000000));
        g.fillRect(thumbX + 1.0f, currentY + 1.0f, thumbWidth, 4.0f);

        // Thumb line - WHITE
        g.setColour(juce::Colours::white);
        g.fillRect(thumbX, currentY - 1.0f, thumbWidth, 4.0f);

        // Thumb border - dark
        g.setColour(juce::Colour(0xFF888888));
        g.drawRect(thumbX, currentY - 1.0f, thumbWidth, 4.0f, 1.0f);

        // Thumb center dot - PINK
        g.setColour(juce::Colour(0xFFFF6B9D));
        g.fillEllipse(bounds.getCentreX() - 5.0f, currentY - 5.0f, 10.0f, 10.0f);

        // Thumb dot highlight
        g.setColour(juce::Colour(0x60FFFFFF));
        g.fillEllipse(bounds.getCentreX() - 3.0f, currentY - 4.0f, 4.0f, 4.0f);
    }

    // ========== TOGGLE SWITCH ==========
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto cornerSize = bounds.getHeight() * 0.5f;

        bool isOn = button.getToggleState();
        auto activeColor = Colors::switchOn;
        auto inactiveColor = Colors::switchOff;

        // Glow for active state
        if (isOn) {
            g.setColour(Colors::glowPink);
            g.fillRoundedRectangle(bounds.expanded(3.0f), cornerSize + 3.0f);
        }

        // Track
        g.setColour(isOn ? activeColor : inactiveColor);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Track border
        g.setColour(isOn ? activeColor.darker(0.1f) : inactiveColor.darker(0.1f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Circle indicator
        auto circleSize = bounds.getHeight() * 0.7f;
        auto circleY = bounds.getCentreY() - circleSize * 0.5f;
        float circleX = isOn ? (bounds.getRight() - circleSize - 2.0f)
                              : (bounds.getX() + 2.0f);

        // Circle shadow
        g.setColour(juce::Colour(0x30000000));
        g.fillEllipse(circleX + 1.0f, circleY + 2.0f, circleSize, circleSize);

        // Circle body
        juce::ColourGradient circleGrad(juce::Colours::white,
                                         circleX, circleY,
                                         juce::Colour(0xFFE0E0E0),
                                         circleX + circleSize, circleY + circleSize, true);
        g.setGradientFill(circleGrad);
        g.fillEllipse(circleX, circleY, circleSize, circleSize);

        // Circle highlight (kawaii shine)
        g.setColour(juce::Colour(0x60FFFFFF));
        g.fillEllipse(circleX + 2.0f, circleY + 1.0f, circleSize * 0.4f, circleSize * 0.3f);
    }

    // ========== BUTTON ==========
    void drawButtonBackground(juce::Graphics& g, juce::Button& /*button*/,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = g.getClipBounds().toFloat();
        auto cornerSize = 8.0f;

        auto colour = backgroundColour;
        if (shouldDrawButtonAsDown) colour = colour.darker(0.05f);
        if (shouldDrawButtonAsHighlighted) colour = colour.brighter(0.05f);

        // Shadow
        g.setColour(juce::Colour(0x15000000));
        g.fillRoundedRectangle(bounds.translated(1.0f, 2.0f), cornerSize);

        // Body
        g.setColour(colour);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(Colors::borderMedium);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Top highlight
        g.setColour(juce::Colour(0x15FFFFFF));
        g.fillRoundedRectangle(bounds.removeFromTop(bounds.getHeight() * 0.3f), cornerSize);
    }

    // ========== COMBO BOX ==========
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
        auto cornerSize = 6.0f;

        // Shadow
        g.setColour(juce::Colour(0x10000000));
        g.fillRoundedRectangle(bounds.translated(1.0f, 2.0f), cornerSize);

        // Background
        g.setColour(Colors::panelBg.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(Colors::borderMedium);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Arrow
        auto arrowBounds = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                                   static_cast<float>(buttonW), static_cast<float>(buttonH));
        auto arrowX = arrowBounds.getCentreX();
        auto arrowY = arrowBounds.getCentreY();

        juce::Path arrow;
        arrow.addTriangle(arrowX - 4.0f, arrowY - 2.0f, arrowX + 4.0f, arrowY - 2.0f,
                          arrowX, arrowY + 3.0f);
        g.setColour(Colors::sakuraPink);
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox& /*box*/) override {
        return juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold));
    }

    // ========== GROUP COMPONENT - Modular Style ==========
    void drawGroupComponentOutline(juce::Graphics& g, int /*width*/, int /*height*/,
                                   const juce::String& text, const juce::Justification&,
                                   juce::GroupComponent& group) override
    {
        auto bounds = group.getLocalBounds().toFloat();
        auto cornerSize = 8.0f;

        // Background - semi-transparent so child controls show through
        g.setColour(juce::Colour(0x10FF8FAB));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border - clean modular style
        g.setColour(juce::Colour(0xFFE0E0E0));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Top accent line (module header indicator)
        if (text.isNotEmpty()) {
            g.setColour(Colors::sakuraPink);
            g.fillRect(bounds.getX() + cornerSize, bounds.getY(), bounds.getWidth() - cornerSize * 2, 2.0f);
        }

        // Title
        if (text.isNotEmpty()) {
            auto font = juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::bold));
            auto textWidth = font.getStringWidth(text);
            auto textBounds = juce::Rectangle<float>(12.0f, 6.0f, textWidth + 8.0f, font.getHeight() + 4.0f);

            // Title text - dark gray
            g.setColour(juce::Colour(0xFF555555));
            g.setFont(font);
            g.drawText(text, textBounds, juce::Justification::centredLeft);
        }
    }

    // ========== UTILITY ==========
    juce::Font getLabelFont(juce::Label& /*label*/) override {
        return juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold));
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override {
        auto layout = LookAndFeel_V4::getSliderLayout(slider);
        layout.textBoxBounds = slider.getLocalBounds().removeFromBottom(22);
        return layout;
    }
};
