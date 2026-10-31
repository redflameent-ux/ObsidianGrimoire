#pragma once
#include "PluginProcessor.h"
#include "../redflame/RedFlameTopBar.h"
#include "../redflame/RedFlameModRail.h"

// Interactive step sequencer grid: lane headers on the left, 16 step cells per lane.
// Click a cell to add/remove a hit; drag to paint with velocity (height = velocity).
class StepGrid : public juce::Component, private juce::Timer
{
public:
    explicit StepGrid (IncantProcessor& p) : proc (p) { startTimerHz (30); }

    std::function<void (int)> onSelect;
    int selected = 0;

    void timerCallback() override { const int s = proc.uiStep.load(); if (s != lastStep) { lastStep = s; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float rowH = b.getHeight() / (float) incant::kLanes;
        const float stepW = (b.getWidth() - labelW) / (float) incant::kSteps;
        const int play = proc.uiStep.load();

        for (int l = 0; l < incant::kLanes; ++l)
        {
            const float y = l * rowH;
            const auto col = juce::Colour (incant::LANES[l].colour);
            const bool mute = proc.pattern.mute[l].load();

            // lane header
            juce::Rectangle<float> label (0, y, (float) labelW, rowH);
            if (l == selected) { g.setColour (col.withAlpha (0.12f)); g.fillRect (label.reduced (1)); }
            g.setColour (rf::hairline); g.drawHorizontalLine ((int) y, 0, b.getWidth());
            g.setColour (mute ? col.withAlpha (0.3f) : col);
            g.fillRoundedRectangle (6.0f, y + rowH * 0.5f - 7.0f, 4.0f, 14.0f, 2.0f);   // mute/colour bar
            g.setColour (rf::textFaint); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText (juce::String (l + 1).paddedLeft ('0', 2), (int) 16, (int) y, 22, (int) rowH, juce::Justification::centredLeft);
            g.setColour (mute ? rf::textFaint : rf::text); g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (incant::LANES[l].name, 40, (int) y, labelW - 46, (int) rowH, juce::Justification::centredLeft);

            // step cells
            for (int s = 0; s < incant::kSteps; ++s)
            {
                juce::Rectangle<float> cell (labelW + s * stepW, y, stepW, rowH);
                cell = cell.reduced (1.6f);
                g.setColour ((s / 4) % 2 == 0 ? rf::panelLo : rf::panel.brighter (0.02f));
                g.fillRoundedRectangle (cell, 2.0f);
                const float v = proc.pattern.get (l, s);
                if (v > 0.0f)
                {
                    const float hh = cell.getHeight() * (0.22f + 0.78f * v);
                    auto hit = cell.withTop (cell.getBottom() - hh);
                    g.setColour (col.withAlpha (mute ? 0.3f : 0.95f));
                    g.fillRoundedRectangle (hit, 2.0f);
                }
                if (s == play) { g.setColour (juce::Colours::white.withAlpha (0.14f)); g.fillRoundedRectangle (cell, 2.0f); }
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.x < labelW)
        {
            const int lane = juce::jlimit (0, incant::kLanes - 1, (int) (e.y / (getHeight() / (float) incant::kLanes)));
            if (e.x < 14) proc.pattern.mute[lane].store (! proc.pattern.mute[lane].load());
            selected = lane; if (onSelect) onSelect (lane);
            repaint(); return;
        }
        paintCell (e, true);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (e.x >= labelW) paintCell (e, false); }

private:
    void paintCell (const juce::MouseEvent& e, bool allowRemove)
    {
        const float rowH = getHeight() / (float) incant::kLanes;
        const float stepW = (getWidth() - labelW) / (float) incant::kSteps;
        const int lane = juce::jlimit (0, incant::kLanes - 1, (int) (e.y / rowH));
        const int step = juce::jlimit (0, incant::kSteps - 1, (int) ((e.x - labelW) / stepW));
        const float rowTop = lane * rowH;
        if (allowRemove && proc.pattern.get (lane, step) > 0.0f) { proc.pattern.set (lane, step, 0.0f); repaint(); return; }
        const float v = juce::jlimit (0.15f, 1.0f, 1.0f - (e.y - rowTop) / rowH);
        proc.pattern.set (lane, step, v);
        repaint();
    }

    IncantProcessor& proc;
    int lastStep = -1;
    static constexpr int labelW = 150;
};

// =====================================================================================
class IncantEditor : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer
{
public:
    explicit IncantEditor (IncantProcessor&);
    ~IncantEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = rf::Knob;
    std::unique_ptr<Knob> knob (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    void refreshDots();
    void selectLane (int l);

    IncantProcessor& proc;
    const juce::Colour acc { rf::accFurrow };
    RedFlameLookAndFeel lnf { rf::accFurrow };

    rf::TopBar topBar;

    // genre strip
    juce::OwnedArray<juce::TextButton> genreBtns;
    int currentGenre = 4;
    rf::Choice chBars { proc.apvts, fid::bars, "Bars" }, chKey { proc.apvts, fid::key, "Key" };
    juce::TextButton genBtn { "GENERATE" }, playBtn { "PLAY" }, clearBtn { "CLEAR" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt;

    StepGrid grid { proc };

    // right column
    rf::Panel pPerform { "Perform", acc }, pGroove { "Groove", acc }, pMacros { "Macros", acc };
    std::unique_ptr<Knob> kEnergy, kComplex, kSwing, kThin, kTiming, kAccent, kM1, kM2, kM3, kM4;
    juce::TextButton regrooveBtn { "REGROOVE" }, buildBtn { "BUILD FROM IT" };

    // lane detail strip
    juce::Label laneLabel;
    juce::Slider levelSlider, panSlider;
    juce::OwnedArray<juce::TextButton> fxBtns;
    juce::Rectangle<int> detailStrip;

    rf::ModRail modRail;
    juce::Rectangle<int> helpStrip;
    std::vector<Knob*> allKnobs;

    std::unique_ptr<rf::PresetManager> presetMgr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IncantEditor)
};
