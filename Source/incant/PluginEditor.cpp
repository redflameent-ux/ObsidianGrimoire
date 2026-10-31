#include "PluginEditor.h"

namespace
{
    void grid4 (juce::Rectangle<int> a, std::vector<juce::Component*> cs, int cols, int pad = 3)
    {
        if (cs.empty()) return;
        const int rows = (int) std::ceil ((double) cs.size() / cols);
        const int cw = a.getWidth() / cols, ch = a.getHeight() / rows;
        for (size_t i = 0; i < cs.size(); ++i)
        {
            const int r = (int) i / cols, c = (int) i % cols;
            cs[i]->setBounds (juce::Rectangle<int> (a.getX() + c * cw, a.getY() + r * ch, cw, ch).reduced (pad));
        }
    }
}

IncantEditor::IncantEditor (IncantProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "INCANT", acc, { "GROOVE", "SAMPLE EDITOR", "FX" }, fid::master),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);

    // genre strip
    const char* genres[] = { "DANCE", "MINIMAL", "PROG", "MELODIC", "TECHNO" };
    for (int i = 0; i < 5; ++i)
    {
        auto* gbt = genreBtns.add (new juce::TextButton (genres[i]));
        gbt->setClickingTogglesState (false);
        gbt->onClick = [this, i] { currentGenre = i; for (int k = 0; k < genreBtns.size(); ++k) genreBtns[k]->setToggleState (k == i, juce::dontSendNotification); };
        addAndMakeVisible (gbt);
    }
    genreBtns[currentGenre]->setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (chBars); addAndMakeVisible (chKey);
    genBtn.onClick = [this] { proc.generate (currentGenre); grid.repaint(); };
    clearBtn.onClick = [this] { proc.pattern.clearAll(); grid.repaint(); };
    playBtn.setClickingTogglesState (true);
    playAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, fid::play, playBtn);
    addAndMakeVisible (genBtn); addAndMakeVisible (playBtn); addAndMakeVisible (clearBtn);

    // grid
    grid.onSelect = [this] (int l) { selectLane (l); };
    addAndMakeVisible (grid);

    // right column
    for (auto* pnl : { &pPerform, &pGroove, &pMacros }) addAndMakeVisible (pnl);
    kEnergy = knob (pPerform, fid::energy, "Energy"); kComplex = knob (pPerform, fid::complexr, "Complex");
    kSwing = knob (pPerform, fid::swing, "Swing"); kThin = knob (pPerform, fid::thinThk, "Thin/Thk");
    kTiming = knob (pGroove, fid::timing, "Timing"); kAccent = knob (pGroove, fid::accent, "Accent");
    pGroove.addAndMakeVisible (regrooveBtn); pGroove.addAndMakeVisible (buildBtn);
    regrooveBtn.onClick = [this] { proc.generate (currentGenre); grid.repaint(); };
    kM1 = knob (pMacros, fid::m1, "M1"); kM2 = knob (pMacros, fid::m2, "M2");
    kM3 = knob (pMacros, fid::m3, "M3"); kM4 = knob (pMacros, fid::m4, "M4");

    // lane detail
    laneLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    laneLabel.setColour (juce::Label::textColourId, rf::text);
    addAndMakeVisible (laneLabel);
    auto setupMini = [this] (juce::Slider& s, bool bipolar)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::rotarySliderFillColourId, acc);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setRange (bipolar ? -1.0 : 0.0, 1.0, 0.01);
        addAndMakeVisible (s);
    };
    setupMini (levelSlider, false); setupMini (panSlider, true);
    levelSlider.onValueChange = [this] { proc.pattern.level[grid.selected].store ((float) levelSlider.getValue()); };
    panSlider.onValueChange   = [this] { proc.pattern.pan[grid.selected].store ((float) panSlider.getValue()); };

    for (auto* nm : { "WSHEAR", "RING", "LO-FI", "COMP", "DELAY", "VERB" })
    {
        auto* fb = fxBtns.add (new juce::TextButton (nm));
        fb->setClickingTogglesState (true);
        addAndMakeVisible (fb);
    }

    addAndMakeVisible (modRail);
    topBar.onTab = [] (int) {};
    selectLane (0);

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<IncantEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    setSize (1480, 920);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1240, 800, 2400, 1500);
}

IncantEditor::~IncantEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> IncantEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void IncantEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void IncantEditor::selectLane (int l)
{
    laneLabel.setText ("LANE " + juce::String (l + 1) + "   " + incant::LANES[l].name, juce::dontSendNotification);
    levelSlider.setValue (proc.pattern.level[l].load(), juce::dontSendNotification);
    panSlider.setValue (proc.pattern.pan[l].load(), juce::dontSendNotification);
}

void IncantEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);

    // card behind the grid + detail strip
    g.setColour (rf::panel);
    g.fillRoundedRectangle (grid.getBounds().toFloat().expanded (0.0f), rf::m::cardRadius);
    g.setColour (rf::border);
    g.drawRoundedRectangle (grid.getBounds().toFloat(), rf::m::cardRadius, 1.0f);

    g.setColour (rf::panel); g.fillRoundedRectangle (detailStrip.toFloat(), rf::m::cardRadius);
    g.setColour (rf::border); g.drawRoundedRectangle (detailStrip.toFloat(), rf::m::cardRadius, 1.0f);
    g.setColour (rf::textFaint); g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("MIX", detailStrip.getX() + 190, detailStrip.getY() + 6, 60, 10, juce::Justification::centredLeft);
    g.drawText ("FX (THIS LANE)", detailStrip.getRight() - 360, detailStrip.getY() + 6, 200, 10, juce::Justification::centredLeft);

    g.setColour (rf::textDim); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("CLICK A STEP TO ADD / REMOVE. DRAG UP / DOWN = VELOCITY. Drag a chip onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("GROOVE GENERATOR  v0.1.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void IncantEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));

    // genre strip
    auto gs = r.removeFromTop (42).reduced (10, 7);
    for (auto* gb : genreBtns) { gb->setBounds (gs.removeFromLeft (78).reduced (1)); }
    gs.removeFromLeft (10);
    chBars.setBounds (gs.removeFromLeft (92)); chKey.setBounds (gs.removeFromLeft (76));
    playBtn.setBounds (gs.removeFromRight (70).reduced (1));
    genBtn.setBounds (gs.removeFromRight (110).reduced (1));
    clearBtn.setBounds (gs.removeFromRight (66).reduced (1));

    modRail.setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (24);
    detailStrip = r.removeFromBottom (80).reduced (10, 4);

    auto main = r.reduced (10, 8);
    auto rightCol = main.removeFromRight (320);
    main.removeFromRight (10);
    grid.setBounds (main.reduced (2));

    // right column panels
    const int rg = 8, h = rightCol.getHeight();
    pPerform.setBounds (rightCol.removeFromTop ((int) ((h - 2 * rg) * 0.4f))); rightCol.removeFromTop (rg);
    pGroove.setBounds (rightCol.removeFromTop ((int) ((h - 2 * rg) * 0.3f))); rightCol.removeFromTop (rg);
    pMacros.setBounds (rightCol);

    grid4 (pPerform.content(), { kEnergy.get(), kComplex.get(), kSwing.get(), kThin.get() }, 2);
    {
        auto c = pGroove.content();
        grid4 (c.removeFromTop (c.getHeight() - 30), { kTiming.get(), kAccent.get() }, 2);
        auto br = c; regrooveBtn.setBounds (br.removeFromLeft (br.getWidth() / 2).reduced (2)); buildBtn.setBounds (br.reduced (2));
    }
    grid4 (pMacros.content(), { kM1.get(), kM2.get(), kM3.get(), kM4.get() }, 4);

    // lane detail
    {
        auto d = detailStrip.reduced (10, 8);
        laneLabel.setBounds (d.removeFromLeft (150).withHeight (20).translated (0, 6));
        auto mix = d.removeFromLeft (140); mix.removeFromTop (12);
        levelSlider.setBounds (mix.removeFromLeft (64).reduced (4));
        panSlider.setBounds (mix.removeFromLeft (64).reduced (4));
        auto fx = d.removeFromRight (360); fx.removeFromTop (12);
        const int bw = fx.getWidth() / 3;
        for (int i = 0; i < fxBtns.size(); ++i)
        {
            const int row = i / 3, cc = i % 3;
            fxBtns[i]->setBounds (fx.getX() + cc * bw, fx.getY() + row * (fx.getHeight() / 2), bw - 4, fx.getHeight() / 2 - 2);
        }
    }
}
