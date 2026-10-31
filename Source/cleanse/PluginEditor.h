#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// Interactive EQ curve: log-freq grid, faked spectrum, per-band curves + summed white
// curve, draggable band points, and a piano strip. Click/drag a point = freq (x) + gain (y).
class EqCurve : public juce::Component, private juce::Timer
{
public:
    EqCurve (CleanseProcessor& p, int& sel, juce::Colour a) : proc (p), selRef (sel), accent (a) { startTimerHz (24); }
    std::function<void (int)> onSelect;
    void timerCallback() override { phase += 0.05f; repaint(); }

    float freqToX (float f, float w) const { return std::log10 (f / 20.0f) / 3.0f * w; }
    float xToFreq (float x, float w) const { return 20.0f * std::pow (10.0f, x / w * 3.0f); }
    float dbToY (float db, float h) const { return h * (1.0f - (db + 15.0f) / 30.0f); }
    float yToDb (float y, float h) const { return (1.0f - y / h) * 30.0f - 15.0f; }

    static float bandDb (const cleanse::Band& b, float f)
    {
        const float lf = std::log (f), lc = std::log (juce::jlimit (20.0f, 20000.0f, b.freq.load()));
        const float g = b.gain.load(), q = juce::jlimit (0.1f, 10.0f, b.q.load());
        switch (b.type.load())
        {
            case 1:  return juce::jmin (0.0f, (lf - lc) * 14.0f);                           // locut
            case 2:  return juce::jmin (0.0f, (lc - lf) * 14.0f);                            // hicut
            case 3:  return g * 0.5f * (1.0f - std::tanh ((lf - lc) * 1.6f));                // loshelf
            case 4:  return g * 0.5f * (1.0f + std::tanh ((lf - lc) * 1.6f));                // hishelf
            default: return g * std::exp (-std::pow ((lf - lc) * q * 0.9f, 2.0f));           // bell
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto pianoH = 20.0f;
        auto a = r.reduced (10.0f); a.removeFromBottom (pianoH);
        const float w = a.getWidth(), h = a.getHeight();

        // grid
        g.setColour (rf::hairline);
        for (float f : { 100.0f, 1000.0f, 10000.0f }) { const float x = a.getX() + freqToX (f, w); g.drawVerticalLine ((int) x, a.getY(), a.getBottom()); }
        for (int db = -12; db <= 12; db += 6) { const float y = a.getY() + dbToY ((float) db, h); g.drawHorizontalLine ((int) y, a.getX(), a.getRight()); }
        g.setColour (accent.withAlpha (0.4f)); g.drawHorizontalLine ((int) (a.getY() + dbToY (0.0f, h)), a.getX(), a.getRight());

        // faked spectrum
        {
            juce::Path sp; sp.startNewSubPath (a.getX(), a.getBottom());
            for (int i = 0; i <= 90; ++i)
            {
                const float t = i / 90.0f;
                const float v = (std::exp (-t * 1.5f) * 0.7f + 0.15f) * (0.6f + 0.4f * std::sin (phase + t * 14.0f)) * (0.5f + 0.5f * (1.0f - t));
                sp.lineTo (a.getX() + t * w, a.getBottom() - v * h * 0.8f);
            }
            sp.lineTo (a.getRight(), a.getBottom()); sp.closeSubPath();
            g.setColour (accent.withAlpha (0.12f)); g.fillPath (sp);
        }

        // per-band curves + summed white curve
        juce::Path total; bool first = true;
        for (int i = 0; i <= 160; ++i)
        {
            const float x = a.getX() + (float) i / 160.0f * w;
            const float f = xToFreq ((float) i / 160.0f * w, w);
            float sum = 0.0f;
            for (auto& b : proc.eq.bands) if (b.on.load()) sum += bandDb (b, f);
            const float y = a.getY() + dbToY (juce::jlimit (-15.0f, 15.0f, sum), h);
            if (first) { total.startNewSubPath (x, y); first = false; } else total.lineTo (x, y);
        }
        g.setColour (juce::Colours::white.withAlpha (0.9f)); g.strokePath (total, juce::PathStrokeType (2.0f));

        // band points
        for (int i = 0; i < cleanse::kBands; ++i)
        {
            auto& b = proc.eq.bands[i];
            if (! b.on.load()) continue;
            const float x = a.getX() + freqToX (juce::jlimit (20.0f, 20000.0f, b.freq.load()), w);
            const float y = a.getY() + dbToY (juce::jlimit (-15.0f, 15.0f, b.gain.load()), h);
            const auto col = juce::Colour (cleanse::DEFAULTS[i].colour);
            g.setColour (col.withAlpha (i == selRef ? 1.0f : 0.8f));
            g.fillEllipse (x - 8, y - 8, 16, 16);
            g.setColour (i == selRef ? juce::Colours::white : col.darker (0.4f));
            g.drawEllipse (x - 8, y - 8, 16, 16, i == selRef ? 2.0f : 1.0f);
            g.setColour (rf::bg); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText (juce::String (i + 1), (int) x - 8, (int) y - 7, 16, 14, juce::Justification::centred);
        }

        // piano strip
        auto piano = juce::Rectangle<float> (a.getX(), a.getBottom() + 2, w, pianoH - 2);
        g.setColour (juce::Colour { 0xffe9e3d3 }); g.fillRoundedRectangle (piano, 2.0f);
        g.setColour (rf::bg);
        for (int k = 1; k < 52; ++k) g.drawVerticalLine ((int) (piano.getX() + k * piano.getWidth() / 52.0f), piano.getY(), piano.getBottom());
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto a = getLocalBounds().toFloat().reduced (10.0f); a.removeFromBottom (20.0f);
        const float w = a.getWidth(), h = a.getHeight();
        int best = -1; float bd = 18.0f;
        for (int i = 0; i < cleanse::kBands; ++i)
        {
            auto& b = proc.eq.bands[i]; if (! b.on.load()) continue;
            const float x = a.getX() + freqToX (b.freq.load(), w), y = a.getY() + dbToY (b.gain.load(), h);
            const float dist = std::hypot (e.x - x, e.y - y);
            if (dist < bd) { bd = dist; best = i; }
        }
        if (best >= 0) { selRef = best; if (onSelect) onSelect (best); repaint(); }
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (selRef < 0) return;
        auto a = getLocalBounds().toFloat().reduced (10.0f); a.removeFromBottom (20.0f);
        auto& b = proc.eq.bands[selRef];
        b.freq.store (juce::jlimit (20.0f, 20000.0f, xToFreq (e.x - a.getX(), a.getWidth())));
        b.gain.store (juce::jlimit (-15.0f, 15.0f, yToDb (e.y - a.getY(), a.getHeight())));
        if (onSelect) onSelect (selRef);
        repaint();
    }

private:
    CleanseProcessor& proc; int& selRef; juce::Colour accent; float phase = 0.0f;
};

// =====================================================================================
class CleanseEditor : public juce::AudioProcessorEditor,
                      public juce::DragAndDropContainer
{
public:
    explicit CleanseEditor (CleanseProcessor&);
    ~CleanseEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();
    void selectBand (int b);

    CleanseProcessor& proc;
    const juce::Colour acc { rf::accClearing };
    RedFlameLookAndFeel lnf { rf::accClearing };

    rf::TopBar topBar;

    // tools row
    juce::OwnedArray<juce::TextButton> toolBtns;   // FREEZE HIGHLIGHT FIND-CLASHES SKETCH MATCH
    rf::Segmented segBands { proc.apvts, cid::bandCount, acc }, segMode { proc.apvts, cid::eqMode, acc };

    EqCurve curve { proc, selBand, acc };
    int selBand = 2;

    // band list
    juce::OwnedArray<juce::TextButton> bandBtns;

    // band editor
    rf::Panel pBand { "Band", acc }, pOut { "Output", acc };
    juce::Label bandTitle;
    juce::Slider freqSlider, gainSlider, qSlider;
    juce::ComboBox typeBox;
    juce::TextButton bandOn { "ON" }, bandSolo { "SOLO" };

    // output
    std::unique_ptr<Knob> kDrive, kMix, kOutput, kCeiling;
    rf::Choice chStage { proc.apvts, cid::stage, "Stage" }, chChar { proc.apvts, cid::character, "Character" };
    rf::Toggle tgLimiter { proc.apvts, cid::limiter, acc, "Limiter" }, tgAuto { proc.apvts, cid::autoGain, acc, "Auto" };

    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    std::unique_ptr<rf::PresetManager> presetMgr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanseEditor)
};
