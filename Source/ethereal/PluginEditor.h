#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// DEEPFIELD-style galactic globe for the SPIRAL panel: a painterly gas-giant that
// morphs warm (Observatory / A) to cool (Singularity / B) across the panel, with a
// spherical grid, glowing A/B poles, a nebula starfield and the YOU marker.
class GlobeDisplay : public juce::Component, private juce::Timer
{
public:
    GlobeDisplay (juce::Colour a, std::atomic<float>* positionPct = nullptr)
        : accent (a), pos (positionPct) { startTimerHz (30); }
    void timerCallback() override { phase += 0.0025f; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        juce::Path frame; frame.addRoundedRectangle (r, 6.0f);
        g.reduceClipRegion (frame);

        auto a = r;
        const juce::Colour warm { 0xffc8863a }, cool { 0xff6a3fb0 };
        const juce::Colour grid = juce::Colour { 0xffe49ab4 };
        const float seam = pos != nullptr ? juce::jlimit (0.0f, 1.0f, pos->load() * 0.01f) : 0.5f;

        // --- nebula background ---
        g.setColour (juce::Colour { 0xff0d0a16 }); g.fillRect (a);
        auto blob = [&] (float cx, float cy, float rad, juce::Colour col)
        {
            juce::ColourGradient cg (col.withAlpha (0.5f), cx, cy, col.withAlpha (0.0f), cx + rad, cy, true);
            g.setGradientFill (cg); g.fillRect (a);
        };
        blob (a.getX() + a.getWidth() * 0.15f, a.getY() + a.getHeight() * 0.3f, a.getWidth() * 0.5f, juce::Colour { 0xff3a1f52 });
        blob (a.getRight() - a.getWidth() * 0.1f, a.getBottom() - a.getHeight() * 0.2f, a.getWidth() * 0.55f, juce::Colour { 0xff4a2458 });
        blob (a.getX() + a.getWidth() * 0.4f, a.getBottom(), a.getWidth() * 0.4f, juce::Colour { 0xff6b3a2a });

        // --- starfield ---
        juce::Random rng (2029);
        for (int i = 0; i < 110; ++i)
        {
            const float x = a.getX() + rng.nextFloat() * a.getWidth();
            const float y = a.getY() + rng.nextFloat() * a.getHeight();
            const float tw = 0.4f + 0.6f * std::abs (std::sin (phase * 6.0f + i));
            const float sz = rng.nextFloat() < 0.08f ? 2.0f : 1.0f;
            g.setColour (juce::Colours::white.withAlpha (0.25f + 0.5f * tw * rng.nextFloat()));
            g.fillEllipse (x, y, sz, sz);
        }

        // --- sphere ---
        const auto c = a.getCentre();
        const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.46f;
        juce::Rectangle<float> sphere (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addEllipse (sphere); g.reduceClipRegion (clip);

            // warm -> cool across the sphere, seam driven by position
            const float seamX = sphere.getX() + sphere.getWidth() * (0.35f + seam * 0.3f);
            juce::ColourGradient body (warm.darker (0.1f), sphere.getX(), c.y, cool.darker (0.1f), sphere.getRight(), c.y, false);
            body.addColour (juce::jlimit (0.02, 0.98, (double) ((seamX - sphere.getX()) / sphere.getWidth())), juce::Colour { 0xff2a2340 });
            g.setGradientFill (body); g.fillEllipse (sphere);

            // cloud bands
            for (int i = 0; i < 11; ++i)
            {
                const float t = i / 10.0f;
                const float y = sphere.getY() + t * sphere.getHeight();
                juce::Path band;
                band.startNewSubPath (sphere.getX() - 4, y);
                for (int k = 0; k <= 20; ++k)
                {
                    const float xx = sphere.getX() + (float) k / 20.0f * sphere.getWidth();
                    const float yy = y + std::sin (k * 0.7f + i * 1.3f + phase * 4.0f) * sphere.getHeight() * 0.018f;
                    band.lineTo (xx, yy);
                }
                const bool light = (i % 2 == 0);
                g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha (0.06f));
                g.strokePath (band, juce::PathStrokeType (sphere.getHeight() * 0.05f));
            }

            // pole plume (top)
            juce::ColourGradient plume (juce::Colour { 0xffb79cff }.withAlpha (0.6f), c.x, sphere.getY(),
                                        juce::Colours::transparentWhite, c.x, c.y, true);
            g.setGradientFill (plume); g.fillEllipse (sphere);

            // spherical shading: highlight upper-left, dark rim
            juce::ColourGradient shade (juce::Colours::white.withAlpha (0.16f), c.x - R * 0.4f, c.y - R * 0.45f,
                                        juce::Colours::transparentWhite, c.x, c.y + R * 0.2f, true);
            g.setGradientFill (shade); g.fillEllipse (sphere);
            juce::ColourGradient vig (juce::Colours::transparentBlack, c.x, c.y,
                                      juce::Colours::black.withAlpha (0.55f), c.x, c.y + R, true);
            vig.point2 = { c.x + R, c.y };
            g.setGradientFill (vig); g.fillEllipse (sphere);
        }

        // rim light (upper-left arc)
        {
            juce::Path rim; rim.addCentredArc (c.x, c.y, R, R, 0.0f,
                juce::MathConstants<float>::pi * 1.15f, juce::MathConstants<float>::pi * 1.95f, true);
            g.setColour (juce::Colour { 0xffd7b8ff }.withAlpha (0.5f));
            g.strokePath (rim, juce::PathStrokeType (1.4f));
        }

        // --- spherical grid ---
        g.setColour (grid.withAlpha (0.45f));
        g.drawLine (c.x, c.y - R, c.x, c.y + R, 1.0f);                              // meridian
        const float eqRy = R * 0.30f;
        g.drawEllipse (c.x - R, c.y - eqRy, R * 2.0f, eqRy * 2.0f, 1.3f);           // equator
        g.setColour (grid.withAlpha (0.22f));
        g.drawEllipse (c.x - R * 1.14f, c.y - R * 0.5f, R * 2.28f, R, 1.0f);        // outer orbit

        // --- X / Y coordinate axes through the YOU centre ---
        g.setColour (grid.withAlpha (0.16f));
        g.drawLine (a.getX(), c.y, a.getRight(), c.y, 1.0f);                        // X axis
        g.drawLine (c.x, a.getY(), c.x, a.getBottom(), 1.0f);                       // Y axis
        g.setColour (grid.withAlpha (0.40f));
        for (int i = 1; i <= 7; ++i)
        {
            const float dx = i * (a.getWidth()  / 16.0f);
            const float dy = i * (a.getHeight() / 12.0f);
            g.drawLine (c.x + dx, c.y - 3.0f, c.x + dx, c.y + 3.0f);
            g.drawLine (c.x - dx, c.y - 3.0f, c.x - dx, c.y + 3.0f);
            g.drawLine (c.x - 3.0f, c.y + dy, c.x + 3.0f, c.y + dy);
            g.drawLine (c.x - 3.0f, c.y - dy, c.x + 3.0f, c.y - dy);
        }
        g.setColour (grid.withAlpha (0.55f));
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawText ("X", juce::Rectangle<float> (a.getRight() - 16.0f, c.y - 13.0f, 14.0f, 11.0f), juce::Justification::centredRight);
        g.drawText ("Y", juce::Rectangle<float> (c.x + 4.0f, a.getY() + 1.0f, 14.0f, 11.0f), juce::Justification::centredLeft);

        // A / B glowing poles on the equator
        auto pole = [&] (float px)
        {
            juce::ColourGradient halo (juce::Colour { 0xffff8fb0 }.withAlpha (0.7f), px, c.y,
                                       juce::Colours::transparentWhite, px + R * 0.28f, c.y, true);
            g.setGradientFill (halo); g.fillEllipse (px - R * 0.28f, c.y - R * 0.28f, R * 0.56f, R * 0.56f);
            g.setColour (juce::Colours::white); g.fillEllipse (px - 3.5f, c.y - 3.5f, 7.0f, 7.0f);
        };
        pole (c.x - R); pole (c.x + R);

        // orbiting node on the equator
        {
            const float ang = phase * juce::MathConstants<float>::twoPi;
            g.setColour (juce::Colour { 0xffff8fb0 });
            g.fillEllipse (c.x + std::cos (ang) * R - 3.0f, c.y + std::sin (ang) * eqRy - 3.0f, 6.0f, 6.0f);
        }

        // YOU marker
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (c.x - 6, c.y - 5, 12, 10, 1.4f);
        g.drawLine (c.x - 10, c.y, c.x - 6, c.y, 1.2f); g.drawLine (c.x + 6, c.y, c.x + 10, c.y, 1.2f);
        g.setColour (rf::text);
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText ("YOU", juce::Rectangle<float> (c.x - 20, c.y + 8, 40, 12), juce::Justification::centred);

        // A / B labels
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.setColour (warm.brighter (0.3f));
        g.drawText ("A  OBSERVATORY", a.reduced (10, 8), juce::Justification::bottomLeft);
        g.setColour (juce::Colour { 0xffb79cff });
        g.drawText ("SINGULARITY  B", a.reduced (10, 8), juce::Justification::bottomRight);

        g.setColour (rf::border); g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
private:
    juce::Colour accent; std::atomic<float>* pos; float phase = 0.0f;
};

// small FX displays (tail / partials / analyzer)
class FxDisplay : public juce::Component, private juce::Timer
{
public:
    enum Mode { Tail, Partials, Analyzer };
    FxDisplay (Mode m, juce::Colour a) : mode (m), accent (a) { startTimerHz (20); }
    void timerCallback() override { phase += 0.05f; repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (rf::panelLo); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (rf::border);  g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto a = r.reduced (8.0f);
        if (mode == Tail)
        {
            juce::Path p; p.startNewSubPath (a.getX(), a.getBottom());
            p.lineTo (a.getX() + a.getWidth() * 0.12f, a.getY() + a.getHeight() * 0.15f);
            p.lineTo (a.getX() + a.getWidth() * 0.3f, a.getY() + a.getHeight() * 0.2f);
            p.quadraticTo (a.getX() + a.getWidth() * 0.7f, a.getY() + a.getHeight() * 0.4f, a.getRight(), a.getBottom() - 2);
            juce::Path fill (p); fill.lineTo (a.getX(), a.getBottom()); fill.closeSubPath();
            g.setColour (accent.withAlpha (0.14f)); g.fillPath (fill);
            g.setColour (accent); g.strokePath (p, juce::PathStrokeType (1.6f));
            g.setColour (rf::textDim); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText ("TAIL  RT60 2.50 s", a.removeFromTop (12), juce::Justification::topRight);
        }
        else if (mode == Partials)
        {
            const int n = 16; const float bw = a.getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                const float h = a.getHeight() * std::exp (-i * 0.12f) * (0.5f + 0.5f * std::sin (phase + i));
                g.setColour (accent.withAlpha (0.8f));
                g.fillRect (a.getX() + i * bw + bw * 0.3f, a.getBottom() - h, bw * 0.4f, h);
            }
        }
        else
        {
            juce::Path p; p.startNewSubPath (a.getX(), a.getBottom());
            for (int i = 0; i <= 60; ++i)
            {
                const float t = i / 60.0f;
                const float v = std::exp (-t * 2.5f) * (0.6f + 0.4f * std::sin (phase * 2.0f + t * 20.0f));
                p.lineTo (a.getX() + t * a.getWidth(), a.getBottom() - v * a.getHeight() * 0.9f);
            }
            p.lineTo (a.getRight(), a.getBottom()); p.closeSubPath();
            g.setColour (accent.withAlpha (0.35f)); g.fillPath (p);
        }
    }
private:
    Mode mode; juce::Colour accent; float phase = 0.0f;
};

// =====================================================================================
class EtherealEditor : public juce::AudioProcessorEditor,
                       public juce::DragAndDropContainer
{
public:
    explicit EtherealEditor (EtherealProcessor&);
    ~EtherealEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();

    EtherealProcessor& proc;
    const juce::Colour acc { rf::accDeepfield };
    RedFlameLookAndFeel lnf { rf::accDeepfield };

    rf::TopBar topBar;

    rf::Panel pStack { "Stack", acc }, pShift { "Galactic Shift", acc }, pSpiral { "Spiral", acc },
              pSpace { "Space", acc }, pInput { "Input", acc }, pOutput { "Output", acc },
              pPart { "Partials", acc }, pRumble { "Rumble", acc };

    // STACK
    rf::Choice chSlotA { proc.apvts, eid::slotA, "Slot A" }, chSlotB { proc.apvts, eid::slotB, "Slot B" };
    rf::Segmented segRouting { proc.apvts, eid::routing, acc, juce::StringArray { "PARALLEL", "SERIES" } };
    std::unique_ptr<Knob> kLevelA, kLevelB;

    // GALACTIC SHIFT
    rf::Segmented segShift { proc.apvts, eid::shiftMode, acc, juce::StringArray { "SMOOTH", "TRIANGLE", "DRIFT" } };
    std::unique_ptr<Knob> kMorph, kOrbit, kReach;

    // SPIRAL
    GlobeDisplay globe { acc, proc.apvts.getRawParameterValue (eid::position) };
    rf::Choice chPath { proc.apvts, eid::path, "Path" };
    juce::Slider posSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> posAtt;
    std::unique_ptr<Knob> kSpin, kSpiral, kHeight;

    // SPACE
    FxDisplay tail { FxDisplay::Tail, acc };
    std::unique_ptr<Knob> kSize, kDecay, kMass, kDamp, kDiffuse, kMod, kDrift;

    // INPUT
    std::unique_ptr<Knob> kDrive, kPre, kLoCut, kHiCut;

    // OUTPUT
    rf::Toggle tgAuto { proc.apvts, eid::autoGain, acc, "Auto Gain" }, tgFreeze { proc.apvts, eid::freeze, acc, "Freeze" };
    std::unique_ptr<Knob> kWidth, kMakeup, kComp, kEclipse, kMix;

    // PARTIALS
    FxDisplay partDisp { FxDisplay::Partials, acc };
    rf::Choice chPartType { proc.apvts, eid::partType, "Type" };
    std::unique_ptr<Knob> kPAmount, kPPitch, kPCount, kPRing, kPShimmer, kPTilt, kPSpread;

    // RUMBLE
    std::unique_ptr<Knob> kRumble, kSub, kRDepth, kRoll, kDark;

    // IN/OUT bar
    FxDisplay analyzer { FxDisplay::Analyzer, acc };
    rf::Segmented segStereo { proc.apvts, eid::stereoMode, acc, juce::StringArray { "STEREO", "MID", "SIDE" } };
    rf::Choice chOversample { proc.apvts, eid::oversample, "Oversample" };
    rf::Segmented segQuality { proc.apvts, eid::quality, acc, juce::StringArray { "FAST", "LINEAR" } };
    rf::Segmented segHq { proc.apvts, eid::hq, acc, juce::StringArray { "HQ", "ECO" } };
    rf::Toggle tgLimit { proc.apvts, eid::limit, acc, "Limit" };
    std::unique_ptr<Knob> kInGain;
    juce::Rectangle<int> ioBar;

    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EtherealEditor)
};
