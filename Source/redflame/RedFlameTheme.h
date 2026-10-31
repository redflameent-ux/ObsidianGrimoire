#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// =====================================================================================
//  RedFlame Synthesizers Co - shared UI kit
//  RedFlameTheme: colour tokens, metrics, and the house LookAndFeel.
//
//  Design language (round two), distilled from the ORACLE / RESONA / DEEPFIELD renders:
//    - near-black page, slightly blue; dark rounded panel "cards" with a short accent
//      header tick + uppercase label.
//    - photoreal brushed-metal knob caps with a fine radial tick ring, a dim track arc
//      and a bright accent value arc (bipolar knobs fill from centre), plus a pointer.
//    - one accent colour per plugin (ORACLE = royal blue).
//    - a bottom modulation rail whose chips are coloured by source family:
//      ENV = gold, LFO = cyan, MACRO = violet, MIDI (velo/wheel/press/key/random) = pink.
// =====================================================================================

namespace rf
{
    // ---- neutrals -------------------------------------------------------------------
    const juce::Colour bg        { 0xff060709 };   // page
    const juce::Colour panel     { 0xff111216 };   // card fill
    const juce::Colour panelHi   { 0xff191a20 };   // raised control fill
    const juce::Colour panelLo   { 0xff0b0c0f };   // inset / well
    const juce::Colour border    { 0xff24252c };   // card + control outline
    const juce::Colour hairline  { 0xff1b1c22 };   // faint divider
    const juce::Colour text      { 0xffe8e9ee };   // near-white label
    const juce::Colour textDim   { 0xff8a8c96 };   // value / secondary
    const juce::Colour textFaint { 0xff55575f };   // tertiary / disabled

    // ---- brushed metal cap ----------------------------------------------------------
    const juce::Colour metalHi   { 0xff787a83 };   // top highlight
    const juce::Colour metalMid  { 0xff44454c };   // body
    const juce::Colour metalLo   { 0xff1c1d22 };   // bottom shade
    const juce::Colour metalRim  { 0xff8a8c95 };   // bright rim

    // ---- modulation-source families (bottom rail) -----------------------------------
    const juce::Colour modEnv    { 0xffe3a73e };   // gold   - envelopes
    const juce::Colour modLfo    { 0xff35c6e8 };   // cyan   - LFOs
    const juce::Colour modMacro  { 0xff9b84ff };   // violet - macros
    const juce::Colour modMidi   { 0xfff2497e };   // pink   - velo / wheel / press / key / random

    // ---- per-plugin accents (set one as the active accent) --------------------------
    const juce::Colour accOracle     { 0xff2f6bff };   // ORACLE   royal blue
    const juce::Colour accResona   { 0xff8f9bf0 };   // RESONA   periwinkle
    const juce::Colour accDeepfield{ 0xff9b7cff };   // DEEPFIELD violet
    const juce::Colour accTenfold  { 0xffff7a3c };   // TENFOLD  orange
    const juce::Colour accEmber    { 0xff3f9bff };   // EMBER    blue
    const juce::Colour accFurrow   { 0xffe3b23e };   // FURROW   gold
    const juce::Colour accClearing { 0xffd9b24a };   // CLEARING gold
    const juce::Colour accSilth    { 0xffc2a06a };   // SILTH    sepia (vintage/lo-fi)

    // ---- metrics --------------------------------------------------------------------
    namespace m
    {
        constexpr float cardRadius   = 10.0f;
        constexpr float ctrlRadius   = 5.0f;
        constexpr int   cardPad      = 14;
        constexpr int   headerH      = 26;   // panel header strip
        constexpr int   topBarH      = 56;
        constexpr int   railH        = 46;   // mod-chip rail
        constexpr int   keyboardH    = 112;
        constexpr int   knobLabelH   = 15;
        constexpr int   knobValueH   = 15;
    }

    // one place to pull the active accent / mod palette by index -----------------------
    inline juce::Colour modFamily (int family) // 0=env 1=lfo 2=macro 3=midi
    {
        switch (family) { case 0: return modEnv; case 1: return modLfo;
                          case 2: return modMacro; default: return modMidi; }
    }
}

// =====================================================================================
//  RedFlameLookAndFeel
// =====================================================================================
class RedFlameLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit RedFlameLookAndFeel (juce::Colour accentColour = rf::accOracle)
        : accent (accentColour)
    {
        setColour (juce::ResizableWindow::backgroundColourId,  rf::bg);
        setColour (juce::ComboBox::backgroundColourId,         rf::panelHi);
        setColour (juce::ComboBox::outlineColourId,            rf::border);
        setColour (juce::ComboBox::textColourId,               rf::text);
        setColour (juce::ComboBox::arrowColourId,              rf::textDim);
        setColour (juce::PopupMenu::backgroundColourId,        rf::panel);
        setColour (juce::PopupMenu::textColourId,              rf::text);
        setColour (juce::PopupMenu::headerTextColourId,        rf::textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId,   rf::text);
        setColour (juce::Label::textColourId,                  rf::text);
        setColour (juce::TextEditor::backgroundColourId,       rf::panelHi);
        setColour (juce::TextEditor::textColourId,             rf::text);
        setColour (juce::TextEditor::outlineColourId,          rf::border);
        setColour (juce::TextEditor::focusedOutlineColourId,   accent);
        setColour (juce::TextButton::buttonColourId,           rf::panelHi);
        setColour (juce::TextButton::textColourOffId,          rf::text);
        setColour (juce::TextButton::textColourOnId,           rf::bg);
        setColour (juce::Slider::textBoxTextColourId,          rf::text);
        setColour (juce::Slider::textBoxBackgroundColourId,    rf::panelHi);
        setColour (juce::Slider::textBoxOutlineColourId,       rf::border);
        setColour (juce::Slider::rotarySliderFillColourId,     accent);
        setColour (juce::TooltipWindow::backgroundColourId,    rf::panelHi);
        setColour (juce::TooltipWindow::textColourId,          rf::text);
    }

    juce::Colour getAccent() const noexcept { return accent; }
    void setAccent (juce::Colour c) noexcept
    {
        accent = c;
        setColour (juce::Slider::rotarySliderFillColourId, accent);
    }

    // ---- the signature brushed-metal rotary ----------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider& s) override
    {
        const auto arcCol = s.findColour (juce::Slider::rotarySliderFillColourId);
        const bool on     = s.isEnabled();

        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float size = juce::jmin (area.getWidth(), area.getHeight());
        area = area.withSizeKeepingCentre (size, size);
        const auto c   = area.getCentre();
        const float R  = size * 0.5f;

        const float arcR  = R - 2.0f;             // outer value/track arc
        const float tickR = R - 5.0f;             // tick ring sits just inside the arc
        const float capR  = R * 0.60f;            // metal cap
        const float ang   = start + pos * (end - start);

        // --- drop shadow under the cap ---
        {
            juce::Path sh; sh.addEllipse (c.x - capR, c.y - capR + 2.5f, capR * 2.0f, capR * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillPath (sh);
        }

        // --- fine radial tick ring ---
        {
            const int ticks = 44;
            for (int i = 0; i < ticks; ++i)
            {
                const float t  = (float) i / (float) (ticks - 1);
                const float a  = start + t * (end - start);
                const float r0 = tickR - 4.0f, r1 = tickR;
                const float ca = std::cos (a - juce::MathConstants<float>::halfPi);
                const float sa = std::sin (a - juce::MathConstants<float>::halfPi);
                // "halfPi" offset: JUCE rotary angles are measured from 12 o'clock
                g.setColour ((a <= ang ? arcCol : rf::textFaint).withAlpha (a <= ang ? (on ? 0.9f : 0.3f) : 0.35f));
                g.drawLine (c.x + ca * r0, c.y + sa * r0, c.x + ca * r1, c.y + sa * r1, 1.1f);
            }
        }

        // --- dim track arc ---
        {
            juce::Path track;
            track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
            g.setColour (rf::border);
            g.strokePath (track, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // --- bright value arc (bipolar fills from centre) ---
        {
            const bool bipolar = s.getMinimum() < -1.0e-6 && s.getMaximum() > 1.0e-6;
            const float from = bipolar ? (start + end) * 0.5f : start;
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
            g.setColour (arcCol.withAlpha (on ? 1.0f : 0.35f));
            g.strokePath (arc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // --- brushed-metal cap ---
        {
            // vertical sheen: bright at the top, shaded at the bottom (reads as metal)
            juce::ColourGradient grad (rf::metalHi, c.x, c.y - capR,
                                       rf::metalLo, c.x, c.y + capR, false);
            grad.addColour (0.5, rf::metalMid);
            g.setGradientFill (grad);
            g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);

            // bright rim + inner shade for a machined edge
            g.setColour (rf::metalRim.withAlpha (0.8f));
            g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.2f);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawEllipse (c.x - capR + 1.6f, c.y - capR + 1.6f, (capR - 1.6f) * 2.0f, (capR - 1.6f) * 2.0f, 1.0f);

            // soft specular highlight, top-left
            juce::ColourGradient spec (juce::Colours::white.withAlpha (0.18f), c.x - capR * 0.4f, c.y - capR * 0.55f,
                                       juce::Colours::transparentWhite, c.x, c.y + capR * 0.2f, true);
            g.setGradientFill (spec);
            g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        }

        // --- pointer ---
        {
            const float ca = std::cos (ang - juce::MathConstants<float>::halfPi);
            const float sa = std::sin (ang - juce::MathConstants<float>::halfPi);
            const float r0 = capR * 0.30f, r1 = capR * 0.92f;
            g.setColour ((on ? rf::text : rf::textDim));
            g.drawLine (c.x + ca * r0, c.y + sa * r0, c.x + ca * r1, c.y + sa * r1, 2.2f);
            g.setColour (arcCol.withAlpha (on ? 0.9f : 0.3f));
            g.fillEllipse (c.x + ca * r1 - 1.6f, c.y + sa * r1 - 1.6f, 3.2f, 3.2f);
        }
    }

    // ---- pill toggle ---------------------------------------------------------------
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        const auto acc = b.findColour (juce::ToggleButton::tickColourId);
        auto r = b.getLocalBounds().toFloat();
        auto pill = r.removeFromLeft (34.0f).withSizeKeepingCentre (30.0f, 16.0f);
        const bool on = b.getToggleState();
        g.setColour (on ? acc.withAlpha (0.9f) : rf::panelHi);
        g.fillRoundedRectangle (pill, 8.0f);
        g.setColour (over ? rf::textDim : rf::border);
        g.drawRoundedRectangle (pill, 8.0f, 1.0f);
        const float kx = on ? pill.getRight() - 14.0f : pill.getX() + 2.0f;
        g.setColour (on ? rf::bg : rf::textDim);
        g.fillEllipse (kx, pill.getY() + 2.0f, 12.0f, 12.0f);
        if (b.getButtonText().isNotEmpty())
        {
            g.setColour (on ? rf::text : rf::textDim);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (b.getButtonText(), r.reduced (6.0f, 0.0f), juce::Justification::centredLeft);
        }
    }

    // ---- combo box -----------------------------------------------------------------
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (rf::panelHi);
        g.fillRoundedRectangle (r, rf::m::ctrlRadius);
        g.setColour (box.hasKeyboardFocus (false) ? accent : rf::border);
        g.drawRoundedRectangle (r, rf::m::ctrlRadius, 1.0f);
        juce::Path arrow;
        const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.5f, juce::Font::bold); }
    juce::Font getPopupMenuFont()               override { return juce::FontOptions (13.5f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::FontOptions (12.0f, juce::Font::bold); }

    // ---- flat dark buttons; accent when "on" (toggle) ------------------------------
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool lit = b.getToggleState();
        juce::Colour fill = lit ? accent.withAlpha (0.92f)
                                 : (down ? rf::border : (over ? rf::panelHi.brighter (0.1f) : rf::panelHi));
        g.setColour (fill);
        g.fillRoundedRectangle (r, rf::m::ctrlRadius);
        g.setColour (lit ? accent : rf::border);
        g.drawRoundedRectangle (r, rf::m::ctrlRadius, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour (b.getToggleState() ? rf::bg : b.findColour (juce::TextButton::textColourOffId));
        g.setFont (getTextButtonFont (b, b.getHeight()));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 0),
                          juce::Justification::centred, 1, 0.9f);
    }

private:
    juce::Colour accent;
};
