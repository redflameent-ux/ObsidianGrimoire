#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// ---- stylized inset displays (visual stand-ins for the live visualizers) ------------
class Display : public juce::Component
{
public:
    enum Mode { Wavetable, Harmonics, Filter, Pad };
    Display (Mode m, juce::Colour a) : mode (m), accent (a) {}

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto in = r.reduced (8.0f);
        if      (mode == Wavetable) paintWavetable (g, in);
        else if (mode == Harmonics) paintHarmonics (g, in);
        else if (mode == Filter)    paintFilter (g, in);
        else                        paintPad (g, in);
    }

private:
    void paintWavetable (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const int frames = 14;
        for (int f = frames - 1; f >= 0; --f)
        {
            const float t  = (float) f / (frames - 1);
            const float yo = a.getY() + t * a.getHeight() * 0.55f + a.getHeight() * 0.12f;
            const float xo = a.getX() + t * a.getWidth() * 0.10f;
            const float ww = a.getWidth() * 0.85f;
            juce::Path p;
            for (int i = 0; i <= 40; ++i)
            {
                const float x = xo + (float) i / 40.0f * ww;
                const float ph = (float) i / 40.0f * juce::MathConstants<float>::twoPi * 2.0f;
                const float amp = (0.5f + 0.5f * t) * a.getHeight() * 0.14f;
                const float y = yo - std::sin (ph + t * 3.0f) * amp * (1.0f - t * 0.5f);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            const bool front = (f == 0);
            g.setColour (front ? accent : accent.withAlpha (0.12f + 0.25f * (1.0f - t)));
            g.strokePath (p, juce::PathStrokeType (front ? 2.0f : 1.0f));
        }
        g.setColour (rf::textDim);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText ("FRAME 6.2 / 64", a.removeFromTop (14), juce::Justification::topLeft);
        g.setColour (rf::textFaint);
        g.drawText ("32-BIT FLOAT", getLocalBounds().reduced (12).removeFromTop (14), juce::Justification::topRight);
    }
    void paintHarmonics (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const int bars = 8;
        const float bw = a.getWidth() / bars;
        for (int i = 0; i < bars; ++i)
        {
            const float h = a.getHeight() * (i == 0 ? 0.9f : std::pow (0.55f, (float) i) + 0.04f);
            auto bar = juce::Rectangle<float> (a.getX() + i * bw + bw * 0.25f, a.getBottom() - h, bw * 0.5f, h);
            g.setColour (i == 1 ? rf::panelHi : accent.withAlpha (0.85f));
            g.fillRoundedRectangle (bar, 1.5f);
        }
    }
    void paintFilter (juce::Graphics& g, juce::Rectangle<float> a)
    {
        juce::Path p;
        p.startNewSubPath (a.getX(), a.getCentreY());
        p.lineTo (a.getX() + a.getWidth() * 0.55f, a.getCentreY());
        p.quadraticTo (a.getX() + a.getWidth() * 0.66f, a.getY() + a.getHeight() * 0.1f,
                       a.getX() + a.getWidth() * 0.72f, a.getCentreY());
        p.quadraticTo (a.getX() + a.getWidth() * 0.82f, a.getBottom(), a.getRight(), a.getBottom());
        g.setColour (accent);
        g.strokePath (p, juce::PathStrokeType (2.0f));
    }
    void paintPad (juce::Graphics& g, juce::Rectangle<float> a)
    {
        g.setColour (rf::hairline);
        for (int i = 1; i < 4; ++i)
        {
            const float x = a.getX() + a.getWidth() * i / 4.0f;
            const float y = a.getY() + a.getHeight() * i / 4.0f;
            g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
            g.drawHorizontalLine ((int) y, a.getX(), a.getRight());
        }
        g.setColour (accent.withAlpha (0.25f));
        g.drawText ("A   E   I   O   U", a, juce::Justification::centredBottom);
    }

    Mode mode;
    juce::Colour accent;
};

// ---- envelope curve (reads the selected env's A/H/D/S/R) -----------------------------
class EnvDisplay : public juce::Component, private juce::Timer
{
public:
    EnvDisplay (OracleProcessor& p, int envIndex, juce::Colour a)
        : proc (p), idx (envIndex), accent (a) { startTimerHz (20); }
    void timerCallback() override { repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (10.0f);

        auto val = [this] (const char* s) { return proc.apvts.getRawParameterValue (envId (idx, s))->load(); };
        const float at = val ("a"), hd = val ("h"), de = val ("d"), su = val ("s") * 0.01f, re = val ("r");
        const float tot = juce::jmax (1.0f, at + hd + de + re + 500.0f);
        auto xAt = [&] (float ms) { return a.getX() + a.getWidth() * (ms / tot); };
        const float top = a.getY(), bot = a.getBottom();
        juce::Path p;
        p.startNewSubPath (a.getX(), bot);
        float x = at;             p.lineTo (xAt (x), top);
        x += hd;                  p.lineTo (xAt (x), top);
        x += de;                  p.lineTo (xAt (x), bot - (bot - top) * su);
        const float sustainEnd = juce::jmin (a.getRight() - a.getWidth() * (re / tot), xAt (x) + a.getWidth() * 0.12f);
        p.lineTo (sustainEnd, bot - (bot - top) * su);
        p.lineTo (a.getRight(), bot);

        juce::Path fill (p); fill.lineTo (a.getX(), bot); fill.closeSubPath();
        g.setColour (accent.withAlpha (0.14f)); g.fillPath (fill);
        g.setColour (accent); g.strokePath (p, juce::PathStrokeType (2.0f));
    }
private:
    OracleProcessor& proc; int idx; juce::Colour accent;
};

// ---- LFO waveform (reads the selected LFO's shape) -----------------------------------
class LfoDisplay : public juce::Component, private juce::Timer
{
public:
    LfoDisplay (OracleProcessor& p, int lfoIndex, juce::Colour a)
        : proc (p), idx (lfoIndex), accent (a) { startTimerHz (30); }
    void timerCallback() override { phase += 0.02f; if (phase > 1.0f) phase -= 1.0f; repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (10.0f);
        const int shape = (int) proc.apvts.getRawParameterValue (lfoId (idx, "shape"))->load();

        juce::Path p;
        const int n = 64;
        for (int i = 0; i <= n; ++i)
        {
            const float t = (float) i / n;
            const float v = wave (shape, t);
            const float x = a.getX() + t * a.getWidth();
            const float y = a.getCentreY() - v * a.getHeight() * 0.42f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (accent); g.strokePath (p, juce::PathStrokeType (2.0f));
        // moving playhead dot
        const float v = wave (shape, phase);
        g.fillEllipse (a.getX() + phase * a.getWidth() - 3.0f,
                       a.getCentreY() - v * a.getHeight() * 0.42f - 3.0f, 6.0f, 6.0f);
    }
private:
    static float wave (int shape, float t)
    {
        switch (shape)
        {
            case 0: return std::sin (t * juce::MathConstants<float>::twoPi);
            case 1: return 1.0f - 4.0f * std::abs (std::round (t - 0.25f) - (t - 0.25f)); // tri
            case 2: return 2.0f * (t - std::floor (t + 0.5f));                            // saw
            case 3: return t < 0.5f ? 1.0f : -1.0f;                                       // square
            default: { juce::Random r ((int) (t * 8)); return r.nextFloat() * 2.0f - 1.0f; }
        }
    }
    OracleProcessor& proc; int idx; juce::Colour accent; float phase = 0.0f;
};

// =====================================================================================
class OracleEditor : public juce::AudioProcessorEditor,
                    public juce::DragAndDropContainer
{
public:
    explicit OracleEditor (OracleProcessor&);
    ~OracleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void selectEnv (int i);
    void selectLfo (int i);
    void refreshDots();

    OracleProcessor& proc;
    RedFlameLookAndFeel lnf { rf::accOracle };
    const juce::Colour acc { rf::accOracle };

    rf::TopBar topBar;

    // ---- upper row ----
    rf::Panel pOscA { "Osc A", acc }, pOscB { "Osc B", acc }, pHarm { "Harmonics", acc },
              pCali { "Cali", acc }, pFilt { "Filter", acc };
    Display viewA { Display::Wavetable, acc }, viewH { Display::Harmonics, acc }, viewF { Display::Filter, acc };
    rf::Toggle aOn { proc.apvts, pid::aOn, acc, " " }, bOn { proc.apvts, pid::bOn, acc, " " };
    rf::Segmented segFilt { proc.apvts, pid::fType, acc };
    std::unique_ptr<Knob> kAwt, kAfine, kAlevel, kAoct, kAsemi;
    std::unique_ptr<Knob> kBwt, kBfine, kBlevel, kBnoise, kBoct, kBsemi;
    std::unique_ptr<Knob> kHcentre, kHwidth, kHspread, kHmix;
    std::unique_ptr<Knob> kCdrive, kCshape, kCbias, kCjitter;
    std::unique_ptr<Knob> kFcut, kFreso, kFdrive, kFenv2, kFkey;

    // ---- lower row ----
    rf::Panel pEnv { "Envelopes", acc }, pLfo { "LFO", acc }, pVoice { "Voice", acc }, pF2 { "Filter 2", acc };

    struct EnvCtl { std::unique_ptr<rf::VSlider> a, h, d, s, r; std::unique_ptr<Knob> velo; };
    EnvCtl env[3];
    std::unique_ptr<EnvDisplay> envDisp[3];
    juce::OwnedArray<juce::TextButton> envTab;
    int curEnv = 0;

    struct LfoCtl { std::unique_ptr<rf::Choice> shape; std::unique_ptr<rf::Toggle> sync, retrig;
                    std::unique_ptr<Knob> rate, smooth, morph, rise; };
    LfoCtl lfo[3];
    std::unique_ptr<LfoDisplay> lfoDisp[3];
    juce::OwnedArray<juce::TextButton> lfoTab;
    int curLfo = 0;

    rf::Segmented segMode { proc.apvts, pid::vMode, acc, juce::StringArray { "POLY", "MONO", "LEGATO" } };
    rf::Toggle mpe { proc.apvts, pid::vMpe, acc, "MPE" };
    std::unique_ptr<Knob> kVoices, kBend, kTune, kGlide, kDrift;

    rf::Segmented segF2 { proc.apvts, pid::f2Type, acc, juce::StringArray { "OFF", "VOWEL", "COMB" } },
                  segRoute { proc.apvts, pid::f2Route, acc, juce::StringArray { "SERIAL", "PARALLEL" } };
    Display vowelPad { Display::Pad, acc };
    std::unique_ptr<Knob> kF2reso, kF2mix, kF2orbit, kF2speed;

    // ---- chrome ----
    juce::MidiKeyboardComponent keyboard;
    rf::ModRail modRail;
    juce::TextButton instLook { "INSTRUMENT LOOK" }, panic { "PANIC" };
    juce::Rectangle<int> helpStrip;

    std::vector<Knob*> allKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OracleEditor)
};
