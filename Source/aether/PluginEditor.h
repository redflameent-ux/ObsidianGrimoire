#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// ---- small displays ------------------------------------------------------------------
class WaveView : public juce::Component
{
public:
    WaveView (AetherProcessor& p, int& sel, juce::Colour a) : proc (p), selRef (sel), accent (a) {}
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (8.0f);
        auto& L = proc.stack.layers[selRef];
        const int wave = L.on.load() ? L.wave.load() : -1;
        // start / end shading
        const float sx = a.getX() + a.getWidth() * (L.start.load() * 0.01f);
        g.setColour (accent.withAlpha (0.08f)); g.fillRect (juce::Rectangle<float> (sx, a.getY(), a.getRight() - sx, a.getHeight()));
        // waveform
        if (wave >= 0)
        {
            juce::Path p; juce::Random rng (7);
            for (int i = 0; i <= 320; ++i)
            {
                const float t = (float) i / 320.0f;
                const float ph = t * 8.0f; const float fr = ph - std::floor (ph);
                float v = 0.0f;
                switch (wave) { case 0: v = 2 * fr - 1; break; case 1: v = fr < 0.5f ? 1 : -1; break;
                                case 2: v = std::sin (fr * juce::MathConstants<float>::twoPi); break;
                                case 3: v = 1 - 4 * std::abs (fr - 0.5f); break; default: v = rng.nextFloat() * 2 - 1; }
                const float env = std::exp (-t * 1.2f);
                const float x = a.getX() + t * a.getWidth();
                const float y = a.getCentreY() - v * env * a.getHeight() * 0.42f;
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (accent); g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        // start / end markers
        g.setColour (juce::Colour { 0xff5fe0a0 }); g.drawLine (sx, a.getY(), sx, a.getBottom(), 1.5f);
        g.setColour (accent.brighter (0.3f)); g.drawLine (a.getRight(), a.getY(), a.getRight(), a.getBottom(), 1.5f);
        g.setColour (rf::textDim); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText ("START", (int) sx + 3, (int) a.getY() + 2, 44, 12, juce::Justification::topLeft);
        g.drawText ("END", (int) a.getRight() - 40, (int) a.getY() + 2, 36, 12, juce::Justification::topRight);
    }
private:
    AetherProcessor& proc; int& selRef; juce::Colour accent;
};

class FilterGraph : public juce::Component, private juce::Timer
{
public:
    FilterGraph (AetherProcessor& p, const char* cutId, const char* resId, const char* typeId, juce::Colour a)
        : proc (p), cut (cutId), res (resId), type (typeId), accent (a) { startTimerHz (15); }
    void timerCallback() override { repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (8.0f);
        const float c = juce::jmap (std::log10 (proc.apvts.getRawParameterValue (cut)->load()), 1.3f, 4.3f, 0.0f, 1.0f);
        const float q = proc.apvts.getRawParameterValue (res)->load() * 0.01f;
        const int tp = (int) proc.apvts.getRawParameterValue (type)->load();
        juce::Path p; p.startNewSubPath (a.getX(), a.getBottom());
        for (int i = 0; i <= 80; ++i)
        {
            const float x = i / 80.0f;
            float g2 = 1.0f;
            const float d = x - c;
            if (tp == 0) g2 = x < c ? 1.0f : std::exp (-(x - c) * 12.0f);          // LP
            else if (tp == 1) g2 = x > c ? 1.0f : std::exp (-(c - x) * 12.0f);     // HP
            else g2 = std::exp (-d * d * 60.0f);                                    // BP
            g2 += q * std::exp (-d * d * 300.0f) * 0.8f;
            p.lineTo (a.getX() + x * a.getWidth(), a.getBottom() - juce::jlimit (0.0f, 1.0f, g2) * a.getHeight());
        }
        g.setColour (accent); g.strokePath (p, juce::PathStrokeType (1.6f));
    }
private:
    AetherProcessor& proc; const char* cut; const char* res; const char* type; juce::Colour accent;
};

class EnvView : public juce::Component, private juce::Timer
{
public:
    EnvView (AetherProcessor& p, juce::Colour a) : proc (p), accent (a) { startTimerHz (15); }
    void timerCallback() override { repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (10.0f);
        auto v = [this] (const char* s) { return proc.apvts.getRawParameterValue (s)->load(); };
        const float at = v (aid::eA), hd = v (aid::eH), de = v (aid::eD), su = v (aid::eS) * 0.01f, re = v (aid::eR);
        const float tot = juce::jmax (1.0f, at + hd + de + re + 500.0f);
        auto X = [&] (float ms) { return a.getX() + a.getWidth() * (ms / tot); };
        juce::Path p; p.startNewSubPath (a.getX(), a.getBottom());
        float x = at; p.lineTo (X (x), a.getY()); x += hd; p.lineTo (X (x), a.getY());
        x += de; p.lineTo (X (x), a.getBottom() - (a.getHeight()) * su);
        const float se = juce::jmin (a.getRight() - a.getWidth() * (re / tot), X (x) + a.getWidth() * 0.15f);
        p.lineTo (se, a.getBottom() - a.getHeight() * su); p.lineTo (a.getRight(), a.getBottom());
        juce::Path fill (p); fill.lineTo (a.getX(), a.getBottom()); fill.closeSubPath();
        g.setColour (accent.withAlpha (0.14f)); g.fillPath (fill);
        g.setColour (accent); g.strokePath (p, juce::PathStrokeType (2.0f));
    }
private:
    AetherProcessor& proc; juce::Colour accent;
};

// ---- the 10-layer stack strip --------------------------------------------------------
class LayersStrip : public juce::Component, private juce::Timer
{
public:
    explicit LayersStrip (AetherProcessor& p, juce::Colour a) : proc (p), accent (a) { startTimerHz (10); }
    std::function<void (int)> onSelect;
    int selected = 0;
    void timerCallback() override { repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float cw = b.getWidth() / (float) aether::kLayers;
        for (int i = 0; i < aether::kLayers; ++i)
        {
            auto card = juce::Rectangle<float> (i * cw, 0, cw, b.getHeight()).reduced (3.0f);
            auto& L = proc.stack.layers[i];
            const bool on = L.on.load();
            g.setColour (i == selected ? rf::panelHi : rf::panel);
            g.fillRoundedRectangle (card, 5.0f);
            g.setColour (i == selected ? accent : rf::border);
            g.drawRoundedRectangle (card, 5.0f, i == selected ? 1.5f : 1.0f);
            auto in = card.reduced (6.0f);
            g.setColour (rf::textDim); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText (juce::String (i + 1), (int) in.getX(), (int) in.getY(), 20, 12, juce::Justification::topLeft);
            // on dot
            auto dot = juce::Rectangle<float> (in.getRight() - 9, in.getY(), 9, 9);
            g.setColour (on ? accent : rf::border); on ? g.fillEllipse (dot) : (void) g.drawEllipse (dot, 1.0f);
            // mini waveform
            auto wv = in.withTrimmedTop (14).withTrimmedBottom (14);
            if (on)
            {
                juce::Path wp; juce::Random rng (i + 3);
                for (int k = 0; k <= 40; ++k)
                {
                    const float t = k / 40.0f; const float fr = (t * 3.0f) - std::floor (t * 3.0f);
                    float vv = 0; const int w = L.wave.load();
                    switch (w) { case 0: vv = 2 * fr - 1; break; case 1: vv = fr < 0.5f ? 1 : -1; break;
                                 case 2: vv = std::sin (fr * juce::MathConstants<float>::twoPi); break;
                                 case 3: vv = 1 - 4 * std::abs (fr - 0.5f); break; default: vv = rng.nextFloat() * 2 - 1; }
                    const float x = wv.getX() + t * wv.getWidth(); const float y = wv.getCentreY() - vv * wv.getHeight() * 0.4f;
                    if (k == 0) wp.startNewSubPath (x, y); else wp.lineTo (x, y);
                }
                g.setColour (accent.withAlpha (0.85f)); g.strokePath (wp, juce::PathStrokeType (1.0f));
            }
            // name
            g.setColour (on ? rf::text : rf::textFaint); g.setFont (juce::FontOptions (9.0f));
            g.drawFittedText (on ? L.name : "empty", (int) in.getX(), (int) in.getBottom() - 12, (int) in.getWidth(), 12, juce::Justification::centredBottom, 1, 0.8f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, aether::kLayers - 1, (int) (e.x / (getWidth() / (float) aether::kLayers)));
        auto card = juce::Rectangle<int> ((int) (i * getWidth() / (float) aether::kLayers), 0, (int) (getWidth() / (float) aether::kLayers), getHeight()).reduced (3);
        if (e.y < card.getY() + 20 && e.x > card.getRight() - 18) proc.stack.layers[i].on.store (! proc.stack.layers[i].on.load());
        selected = i; if (onSelect) onSelect (i); repaint();
    }
private:
    AetherProcessor& proc; juce::Colour accent;
};

// =====================================================================================
class AetherEditor : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer,
                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit AetherEditor (AetherProcessor&);
    ~AetherEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();
    void loadLayer (int n);
    void parameterChanged (const juce::String& id, float value) override;

    AetherProcessor& proc;
    const juce::Colour acc { rf::accTenfold };
    RedFlameLookAndFeel lnf { rf::accTenfold };

    rf::TopBar topBar;
    LayersStrip strip { proc, acc };
    int selLayer = 0;
    bool loading = false;

    rf::Panel pLayer { "Layer", acc }, pFiltA { "Filter A", acc }, pFiltB { "Filter B", acc },
              pPatina { "Patina", acc }, pEnv { "Envelopes", acc }, pMacros { "Macros", acc };

    // LAYER editor
    WaveView wave { proc, selLayer, acc };
    juce::Label layerName;
    juce::OwnedArray<juce::TextButton> layerToggles;   // ON LOOP REVERSE KEY-TRACK AUTO-TUNE
    std::unique_ptr<Knob> kLevel, kPan, kCoarse, kFine, kRoot, kStart, kLoopxf, kDrift, kGsize, kGdensity, kGspray, kGrandom;

    // FILTER A / B
    rf::Toggle tgFAon { proc.apvts, aid::fAon, acc, "On" }, tgFBon { proc.apvts, aid::fBon, acc, "On" };
    rf::Choice chFAtype { proc.apvts, aid::fAtype, "" }, chFBtype { proc.apvts, aid::fBtype, "" };
    FilterGraph graphA { proc, aid::fAcut, aid::fAres, aid::fAtype, acc }, graphB { proc, aid::fBcut, aid::fBres, aid::fBtype, acc };
    rf::Segmented segRoute { proc.apvts, aid::routing, acc };
    std::unique_ptr<Knob> kFAcut, kFAres, kFAdrive, kFAenv2, kFAkey, kFAmix;
    std::unique_ptr<Knob> kFBcut, kFBres, kFBdrive, kFBenv2, kFBkey, kFBmix;

    // PATINA
    rf::Choice chMachine { proc.apvts, aid::pMachine, "Machine" };
    std::unique_ptr<Knob> kPDrift, kPVariance, kPAge, kPVca, kPTape, kPWobble, kPNoise;

    // ENV
    EnvView envView { proc, acc };
    std::unique_ptr<Knob> kEA, kEH, kED, kES, kER, kCurve, kVelo;

    // MACROS
    std::unique_ptr<Knob> kM[8];

    juce::MidiKeyboardComponent keyboard;
    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AetherEditor)
};
