#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// resonator spectrum (animated lollipop partials) - stylized stand-in
class ResDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ResDisplay (juce::Colour a) : accent (a) { startTimerHz (20); }
    void timerCallback() override { phase += 0.03f; repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (10.0f);
        g.setColour (rf::textDim);
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText ("F0 258.3 Hz   PARTIALS 17", a.removeFromTop (14), juce::Justification::topLeft);
        a.removeFromTop (4);
        const int n = 17;
        const float bw = a.getWidth() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const float env = std::exp (-i * 0.14f);
            const float wob = 0.5f + 0.5f * std::sin (phase + i * 0.6f);
            const float h = a.getHeight() * env * (0.4f + 0.6f * wob);
            const float x = a.getX() + i * bw + bw * 0.5f;
            g.setColour (accent.withAlpha (0.9f));
            g.drawLine (x, a.getBottom(), x, a.getBottom() - h, 2.0f);
            g.fillEllipse (x - 2.5f, a.getBottom() - h - 2.5f, 5.0f, 5.0f);
        }
    }
private:
    juce::Colour accent; float phase = 0.0f;
};

// =====================================================================================
class SigilEditor : public juce::AudioProcessorEditor,
                    public juce::DragAndDropContainer
{
public:
    explicit SigilEditor (SigilProcessor&);
    ~SigilEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();

    SigilProcessor& proc;
    const juce::Colour acc { rf::accResona };
    RedFlameLookAndFeel lnf { rf::accResona };

    rf::TopBar topBar;

    // engine strip
    rf::Segmented segEngine { proc.apvts, sid::engine, acc,
        juce::StringArray { "MODAL", "SYMPATHETIC", "INHARMONIC", "FM", "CHORDS" } };
    std::unique_ptr<Knob> kM1, kM2, kM3, kM4;

    // panels
    rf::Panel pExc { "Exciter", acc }, pRes { "Resonator", acc }, pSpace { "Space", acc },
              pMot { "Motion", acc }, pPlay { "Play", acc }, pTouch { "Touch", acc };

    // EXCITER
    std::unique_ptr<Knob> kStrike, kBow, kColor, kOsc, kShape, kAtk, kDec, kSus, kRel;
    rf::Segmented segEnv { proc.apvts, sid::env, acc, juce::StringArray { "AMP", "EXCITER" } };

    // RESONATOR
    ResDisplay resDisp { acc };
    std::unique_ptr<Knob> kStruct, kBright, kDamp, kPos, kKeyUp;
    rf::Choice chChord { proc.apvts, sid::chord, "Chord" }, chKey { proc.apvts, sid::key, "Key" },
               chScale { proc.apvts, sid::scale, "Scale" };

    // SPACE
    rf::Toggle tgChorus { proc.apvts, sid::chorusOn, acc, "Chorus" }, tgReverb { proc.apvts, sid::reverbOn, acc, "Reverb" },
               tgSync { proc.apvts, sid::reverbSync, acc, "Sync" };
    rf::Choice chChorusType { proc.apvts, sid::chorusType, "Type" }, chRvType { proc.apvts, sid::reverbType, "Rev Type" };
    std::unique_ptr<Knob> kChAmt, kChDepth, kChRate, kChWidth, kRvMix, kRvTime, kRvTone;

    // MOTION
    rf::Choice chMotShape { proc.apvts, sid::motShape, "Shape" };
    std::unique_ptr<Knob> kMotRate, kMStruct, kMBright, kMPos, kMVib, kMWt;

    // PLAY
    rf::Segmented segMode { proc.apvts, sid::mode, acc, juce::StringArray { "POLY", "MONO", "LEGATO" } };
    std::unique_ptr<Knob> kVoices, kGlide, kOctave, kTune;

    // TOUCH
    std::unique_ptr<Knob> kVelBrt, kVelStk, kMwLfo, kMwBrt, kAtExc, kBend;

    // chrome
    juce::MidiKeyboardComponent keyboard;
    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SigilEditor)
};
