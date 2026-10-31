#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// saturate spectrum display (animated) with a focus marker
class SpectrumView : public juce::Component, private juce::Timer
{
public:
    SpectrumView (FoundationProcessor& p, juce::Colour a) : proc (p), accent (a) { startTimerHz (20); }
    void timerCallback() override { phase += 0.05f; repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (8.0f);
        // spectrum hump
        juce::Path p; p.startNewSubPath (a.getX(), a.getBottom());
        for (int i = 0; i <= 80; ++i)
        {
            const float t = i / 80.0f;
            const float hump = std::exp (-std::pow ((t - 0.4f) * 3.0f, 2.0f)) * (0.6f + 0.2f * std::sin (phase + t * 8.0f));
            p.lineTo (a.getX() + t * a.getWidth(), a.getBottom() - hump * a.getHeight() * 0.9f);
        }
        p.lineTo (a.getRight(), a.getBottom()); p.closeSubPath();
        g.setColour (accent.withAlpha (0.28f)); g.fillPath (p);
        g.setColour (accent); g.strokePath (p, juce::PathStrokeType (1.4f));
        // focus marker
        const float fc = juce::jmap (std::log10 (proc.apvts.getRawParameterValue (fnd::focus)->load()), 1.3f, 4.3f, 0.0f, 1.0f);
        g.setColour (accent.withAlpha (0.5f)); g.drawLine (a.getX() + fc * a.getWidth(), a.getY(), a.getX() + fc * a.getWidth(), a.getBottom(), 1.0f);
        g.setColour (rf::textDim); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText ("FOCUS 1.00 kHz", a.removeFromTop (12), juce::Justification::topLeft);
        g.drawText ("ROOT AUTO 69 Hz", getLocalBounds().reduced (10).removeFromTop (12), juce::Justification::topRight);
    }
private:
    FoundationProcessor& proc; juce::Colour accent; float phase = 0.0f;
};

// LUFS readout
class LufsReadout : public juce::Component, private juce::Timer
{
public:
    LufsReadout (std::atomic<float>& v, juce::String lbl) : val (v), label (std::move (lbl)) { startTimerHz (10); }
    void timerCallback() override { repaint(); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (rf::textFaint); g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawText (label, getLocalBounds().removeFromTop (12), juce::Justification::centred);
        g.setColour (rf::text); g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText (juce::String (val.load(), 1), getLocalBounds().withTrimmedTop (10).withTrimmedBottom (12), juce::Justification::centred);
        g.setColour (rf::textDim); g.setFont (juce::FontOptions (8.5f));
        g.drawText ("LUFS", getLocalBounds().removeFromBottom (12), juce::Justification::centred);
    }
private:
    std::atomic<float>& val; juce::String label;
};

// horizontal gain-reduction / level meter
class MeterBar : public juce::Component, private juce::Timer
{
public:
    MeterBar (std::atomic<float>& v, juce::String lbl, float maxDb, juce::Colour a) : val (v), label (std::move (lbl)), maxv (maxDb), accent (a) { startTimerHz (20); }
    void timerCallback() override { repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (rf::textFaint); g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawText (label, r.removeFromTop (12).toNearestInt(), juce::Justification::centredLeft);
        g.drawText (juce::String (-val.load(), 1) + " dB", getLocalBounds().removeFromTop (12), juce::Justification::centredRight);
        auto bar = r.reduced (0, 2);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (bar, 2.0f);
        const float amt = juce::jlimit (0.0f, 1.0f, val.load() / maxv);
        g.setColour (accent); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * amt), 2.0f);
    }
private:
    std::atomic<float>& val; juce::String label; float maxv; juce::Colour accent;
};

// =====================================================================================
class FoundationEditor : public juce::AudioProcessorEditor,
                         public juce::DragAndDropContainer
{
public:
    explicit FoundationEditor (FoundationProcessor&);
    ~FoundationEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();

    FoundationProcessor& proc;
    const juce::Colour acc { rf::accEmber };
    RedFlameLookAndFeel lnf { rf::accEmber };

    rf::TopBar topBar;
    rf::Panel pIn { "Input", acc }, pSat { "Saturate", acc }, pComp { "Compress", acc }, pLoud { "Loud", acc }, pOut { "Output", acc };

    // INPUT
    std::unique_ptr<Knob> kInput;
    LufsReadout inLufs { proc.inLufs, "IN" };
    rf::Segmented segQuality { proc.apvts, fnd::quality, acc, juce::StringArray { "OFF", "2X", "4X" } };
    rf::Toggle tgLive { proc.apvts, fnd::live, acc, "Live" };

    // SATURATE
    SpectrumView spectrum { proc, acc };
    std::unique_ptr<Knob> kBass, kThick, kDepth, kSub, kFocus, kSpread, kPhantom, kRoot, kColor;

    // COMPRESS
    rf::Segmented segCompMode { proc.apvts, fnd::compMode, acc, juce::StringArray { "COMP", "OTT" } };
    rf::Toggle tgClip { proc.apvts, fnd::clip, acc, "Clip" };
    std::unique_ptr<Knob> kComp, kBody, kSoft, kCrunch;
    MeterBar reduceMeter { proc.reduceDb, "REDUCE", 24.0f, acc };
    MeterBar limitMeter2 { proc.reduceDb, "SHAVE", 24.0f, acc };

    // LOUD
    std::unique_ptr<Knob> kLoud, kPunch, kClarity, kShape, kWidth, kTarget;
    MeterBar limitMeter { proc.limitDb, "LIMIT", 12.0f, acc };

    // OUTPUT
    std::unique_ptr<Knob> kMix, kOutput;
    LufsReadout outLufs { proc.outLufs, "OUT" };
    juce::TextButton unityBtn { "UNITY" };
    rf::Toggle tgBypass { proc.apvts, fnd::bypass, acc, "Bypass" };

    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    std::unique_ptr<rf::PresetManager> presetMgr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FoundationEditor)
};
