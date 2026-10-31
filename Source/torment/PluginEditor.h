#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// CRT oscilloscope display: brown bezel, dark-green screen, glowing waveform + scanlines.
class CRTDisplay : public juce::Component, private juce::Timer
{
public:
    explicit CRTDisplay (TormentProcessor& p) : proc (p) { startTimerHz (30); }
    void timerCallback() override { phase += 0.08f; repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        // bezel
        g.setColour (juce::Colour { 0xff2a2016 }); g.fillRoundedRectangle (r, 14.0f);
        g.setColour (juce::Colour { 0xff4a3a24 }); g.drawRoundedRectangle (r, 14.0f, 1.5f);
        auto scr = r.reduced (14.0f);
        // screen
        juce::ColourGradient sg (juce::Colour { 0xff0a1410 }, scr.getCentreX(), scr.getCentreY(),
                                 juce::Colour { 0xff060a08 }, scr.getX(), scr.getBottom(), true);
        g.setGradientFill (sg); g.fillRoundedRectangle (scr, 10.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (scr, 10.0f); g.reduceClipRegion (clip);
            // scanlines
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            for (float y = scr.getY(); y < scr.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, scr.getX(), scr.getRight());
            // waveform
            const float amp = juce::jlimit (0.05f, 1.0f, 0.3f + proc.scopeLevel.load() * 1.2f);
            juce::Path wave;
            const juce::Colour glow { 0xff7fe6a8 };
            for (int i = 0; i <= 240; ++i)
            {
                const float t = (float) i / 240.0f;
                const float y = scr.getCentreY() - std::sin (t * juce::MathConstants<float>::twoPi * 7.0f + phase) * scr.getHeight() * 0.3f * amp;
                const float x = scr.getX() + t * scr.getWidth();
                if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
            }
            g.setColour (glow.withAlpha (0.25f)); g.strokePath (wave, juce::PathStrokeType (3.5f));
            g.setColour (glow); g.strokePath (wave, juce::PathStrokeType (1.4f));
            // ground line
            g.setColour (glow.withAlpha (0.4f)); g.drawLine (scr.getX(), scr.getBottom() - 10, scr.getX() + 40, scr.getBottom() - 10, 1.0f);
        }
    }
private:
    TormentProcessor& proc; float phase = 0.0f;
};

// =====================================================================================
class TormentEditor : public juce::AudioProcessorEditor,
                      public juce::DragAndDropContainer
{
public:
    explicit TormentEditor (TormentProcessor&);
    ~TormentEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();

    TormentProcessor& proc;
    const juce::Colour acc { rf::accSilth };
    RedFlameLookAndFeel lnf { rf::accSilth };

    rf::TopBar topBar;

    rf::Panel pAge { "Age", acc }, pScope { "Scope", acc }, pCapture { "Capture", acc },
              pTape { "Tape", acc }, pDecimate { "Decimate", acc }, pVinyl { "Vinyl", acc }, pGrain { "Grain", acc };

    CRTDisplay crt { proc };

    std::unique_ptr<Knob> kAge, kMix, kPunch, kTape, kDec, kVin;
    rf::Segmented segCapLen { proc.apvts, tid::capLen, acc }, segCapMode { proc.apvts, tid::capMode, acc };
    std::unique_ptr<Knob> kBpm;
    juce::TextButton recBtn { "REC NEXT" }, grabBtn { "GRAB LAST" }, wavBtn { "WAV" };

    std::unique_ptr<Knob> kTDrive, kTBump, kTFlutter, kTHiss;

    rf::Choice chMachine { proc.apvts, tid::dMachine, "Machine" };
    rf::Toggle tgUlaw { proc.apvts, tid::dUlaw, acc, "U-Law" };
    std::unique_ptr<Knob> kDRate, kDBits, kDSmooth, kDJitter;

    rf::Segmented segSpeed { proc.apvts, tid::vSpeed, acc };
    std::unique_ptr<Knob> kVDust, kVPops, kVRumble, kVWow, kVTone, kVMono;

    rf::Toggle tgFreeze { proc.apvts, tid::gFreeze, acc, "Freeze" };
    rf::Choice chShape { proc.apvts, tid::gShape, "Shape" }, chScale { proc.apvts, tid::gScale, "Scale" }, chSync { proc.apvts, tid::gSync, "Sync" };
    rf::Segmented segPitchMode { proc.apvts, tid::gPitchMode, acc, juce::StringArray { "HOLD", "BEND" } };
    std::unique_ptr<Knob> kGrain, kGSize, kGDensity, kGSpray, kGDetune, kGWidth, kGReverse, kGPitch, kGWarp;
    std::unique_ptr<Knob> kGPositn, kGFdbk, kGChance, kGOctave, kGJitter, kGColor, kGReso;

    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TormentEditor)
};
