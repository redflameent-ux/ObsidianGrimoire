#include "PluginEditor.h"

namespace
{
    void grid (juce::Rectangle<int> a, std::vector<juce::Component*> cs, int cols, int pad = 3)
    {
        if (cs.empty()) return;
        const int rows = (int) std::ceil ((double) cs.size() / cols);
        const int cw = a.getWidth() / cols;
        const int ch = a.getHeight() / rows;
        for (size_t i = 0; i < cs.size(); ++i)
        {
            const int r = (int) i / cols, c = (int) i % cols;
            cs[i]->setBounds (juce::Rectangle<int> (a.getX() + c * cw, a.getY() + r * ch, cw, ch).reduced (pad));
        }
    }

    // lay a row of panels by weight; returns nothing, mutates `row`
    void layoutPanels (juce::Rectangle<int> row, std::vector<rf::Panel*> panels, std::vector<float> w, int gap)
    {
        float sum = 0; for (float x : w) sum += x;
        const int usable = row.getWidth() - gap * ((int) panels.size() - 1);
        for (size_t i = 0; i < panels.size(); ++i)
        {
            const int px = (i == panels.size() - 1) ? row.getWidth()
                             : (int) std::round (usable * (w[i] / sum));
            panels[i]->setBounds (row.removeFromLeft (px));
            if (i < panels.size() - 1) row.removeFromLeft (gap);
        }
    }
}

OracleEditor::OracleEditor (OracleProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "ORACLE", acc, { "SYNTH", "MOD", "ARP", "FX" }, pid::master),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "MACRO 1", 2 }, { "MACRO 2", 2 }, { "MACRO 3", 2 }, { "MACRO 4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "RANDOM", 3 }, { "H GATE", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);

    // ---- upper row ----
    for (auto* pnl : { &pOscA, &pOscB, &pHarm, &pCali, &pFilt }) addAndMakeVisible (pnl);
    pOscA.addAndMakeVisible (viewA); pHarm.addAndMakeVisible (viewH); pFilt.addAndMakeVisible (viewF);
    pOscA.addAndMakeVisible (aOn);   pOscB.addAndMakeVisible (bOn);   pFilt.addAndMakeVisible (segFilt);

    kAwt = knob (pOscA, pid::aWt); kAfine = knob (pOscA, pid::aFine); kAlevel = knob (pOscA, pid::aLevel);
    kAoct = knob (pOscA, pid::aOct); kAsemi = knob (pOscA, pid::aSemi);
    kBwt = knob (pOscB, pid::bWt); kBfine = knob (pOscB, pid::bFine); kBlevel = knob (pOscB, pid::bLevel);
    kBnoise = knob (pOscB, pid::bNoise); kBoct = knob (pOscB, pid::bOct); kBsemi = knob (pOscB, pid::bSemi);
    kHcentre = knob (pHarm, pid::hCentre); kHwidth = knob (pHarm, pid::hWidth);
    kHspread = knob (pHarm, pid::hSpread); kHmix = knob (pHarm, pid::hMix);
    kCdrive = knob (pCali, pid::cDrive); kCshape = knob (pCali, pid::cShape);
    kCbias = knob (pCali, pid::cBias); kCjitter = knob (pCali, pid::cJitter);
    kFcut = knob (pFilt, pid::fCutoff); kFreso = knob (pFilt, pid::fReso); kFdrive = knob (pFilt, pid::fDrive);
    kFenv2 = knob (pFilt, pid::fEnv2); kFkey = knob (pFilt, pid::fKey);

    // ---- lower row ----
    for (auto* pnl : { &pEnv, &pLfo, &pVoice, &pF2 }) addAndMakeVisible (pnl);

    auto mkV = [this] (rf::Panel& parent, const juce::String& id, const juce::String& label)
    {
        auto v = std::make_unique<rf::VSlider> (proc.apvts, id, acc, label);
        parent.addAndMakeVisible (*v); return v;
    };

    for (int i = 0; i < 3; ++i)
    {
        const int n = i + 1;
        env[i].a = mkV (pEnv, envId (n, "a"), "A"); env[i].h = mkV (pEnv, envId (n, "h"), "H");
        env[i].d = mkV (pEnv, envId (n, "d"), "D"); env[i].s = mkV (pEnv, envId (n, "s"), "S");
        env[i].r = mkV (pEnv, envId (n, "r"), "R");
        env[i].velo = knob (pEnv, envId (n, "v"), "Velo");
        envDisp[i] = std::make_unique<EnvDisplay> (proc, n, acc); pEnv.addAndMakeVisible (*envDisp[i]);

        auto* et = envTab.add (new juce::TextButton ("ENV " + juce::String (n)));
        et->setClickingTogglesState (false); et->onClick = [this, i] { selectEnv (i); };
        pEnv.addAndMakeVisible (et);

        lfo[i].shape  = std::make_unique<rf::Choice> (proc.apvts, lfoId (n, "shape"), "Shape");
        lfo[i].sync   = std::make_unique<rf::Toggle> (proc.apvts, lfoId (n, "sync"), acc, "Sync");
        lfo[i].retrig = std::make_unique<rf::Toggle> (proc.apvts, lfoId (n, "retrig"), acc, "Retrig");
        pLfo.addAndMakeVisible (*lfo[i].shape); pLfo.addAndMakeVisible (*lfo[i].sync); pLfo.addAndMakeVisible (*lfo[i].retrig);
        lfo[i].rate = knob (pLfo, lfoId (n, "rate"), "Rate"); lfo[i].smooth = knob (pLfo, lfoId (n, "smooth"), "Smooth");
        lfo[i].morph = knob (pLfo, lfoId (n, "morph"), "Morph"); lfo[i].rise = knob (pLfo, lfoId (n, "rise"), "Rise");
        lfoDisp[i] = std::make_unique<LfoDisplay> (proc, n, acc); pLfo.addAndMakeVisible (*lfoDisp[i]);

        auto* lt = lfoTab.add (new juce::TextButton ("LFO " + juce::String (n)));
        lt->setClickingTogglesState (false); lt->onClick = [this, i] { selectLfo (i); };
        pLfo.addAndMakeVisible (lt);
    }

    pVoice.addAndMakeVisible (segMode); pVoice.addAndMakeVisible (mpe);
    kVoices = knob (pVoice, pid::vVoices, "Voices"); kBend = knob (pVoice, pid::vBend, "Bend");
    kTune = knob (pVoice, pid::vTune, "Tune"); kGlide = knob (pVoice, pid::vGlide, "Glide");
    kDrift = knob (pVoice, pid::vDrift, "Drift");

    pF2.addAndMakeVisible (segF2); pF2.addAndMakeVisible (segRoute); pF2.addAndMakeVisible (vowelPad);
    kF2reso = knob (pF2, pid::f2Reso, "Reso"); kF2mix = knob (pF2, pid::f2Mix, "Mix");
    kF2orbit = knob (pF2, pid::f2Orbit, "Orbit"); kF2speed = knob (pF2, pid::f2Speed, "Speed");

    // ---- chrome ----
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, rf::panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, rf::panelLo);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, rf::bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, acc.withAlpha (0.8f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, acc.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setKeyWidth (20.0f);
    keyboard.setAvailableRange (24, 96);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (modRail);

    instLook.setClickingTogglesState (true); addAndMakeVisible (instLook);
    panic.onClick = [this] { proc.keyboardState.allNotesOff (0); };
    addAndMakeVisible (panic);
    topBar.onTab = [] (int) {};

    selectEnv (0); selectLfo (0);

    // mod dots: rebuild all knobs whenever an assignment is added/removed/loaded
    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<OracleEditor> (this)]
    {
        juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); });
    };
    refreshDots();

    setSize (1360, 884);
    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setSizeLimits (1150, 760, 2300, 1500);
}

OracleEditor::~OracleEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> OracleEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void OracleEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void OracleEditor::selectEnv (int i)
{
    curEnv = i;
    for (int j = 0; j < 3; ++j)
    {
        const bool on = (j == i);
        juce::Component* cs[] = { env[j].a.get(), env[j].h.get(), env[j].d.get(),
                                  env[j].s.get(), env[j].r.get(), env[j].velo.get(), envDisp[j].get() };
        for (auto* c : cs) if (c) c->setVisible (on);
        envTab[j]->setToggleState (on, juce::dontSendNotification);
    }
}

void OracleEditor::selectLfo (int i)
{
    curLfo = i;
    for (int j = 0; j < 3; ++j)
    {
        const bool on = (j == i);
        juce::Component* cs[] = { lfo[j].shape.get(), lfo[j].sync.get(), lfo[j].retrig.get(),
                                  lfo[j].rate.get(), lfo[j].smooth.get(), lfo[j].morph.get(),
                                  lfo[j].rise.get(), lfoDisp[j].get() };
        for (auto* c : cs) if (c) c->setVisible (on);
        lfoTab[j]->setToggleState (on, juce::dontSendNotification);
    }
}

void OracleEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    g.setColour (rf::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Welcome to ORACLE. Hover over anything and this bar explains it. Drag a chip from the row below onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("HYBRID WAVETABLE SYNTHESIZER  v0.2.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void OracleEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));
    keyboard.setBounds (r.removeFromBottom (rf::m::keyboardH));
    modRail .setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (26);
    {
        auto hs = helpStrip.withSizeKeepingCentre (236, 20);
        instLook.setBounds (hs.removeFromLeft (156)); hs.removeFromLeft (6); panic.setBounds (hs);
    }

    auto content = r.reduced (10, 8);
    const int rowGap = 8;
    auto upper = content.removeFromTop ((content.getHeight() - rowGap) / 2);
    content.removeFromTop (rowGap);
    auto lower = content;

    layoutPanels (upper, { &pOscA, &pOscB, &pHarm, &pCali, &pFilt }, { 1.35f, 1.0f, 1.05f, 0.68f, 1.3f }, 10);
    layoutPanels (lower, { &pEnv, &pLfo, &pVoice, &pF2 },            { 1.35f, 1.15f, 1.0f, 1.2f }, 10);

    // ---- upper panels ----
    { auto c = pOscA.content(); aOn.setBounds (c.removeFromTop (20).removeFromLeft (44));
      viewA.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.42f))); c.removeFromTop (6);
      grid (c, { kAwt.get(), kAfine.get(), kAlevel.get(), kAoct.get(), kAsemi.get() }, 5); }
    { auto c = pOscB.content(); bOn.setBounds (c.removeFromTop (20).removeFromLeft (44)); c.removeFromTop (4);
      grid (c, { kBwt.get(), kBfine.get(), kBlevel.get(), kBnoise.get(), kBoct.get(), kBsemi.get() }, 3); }
    { auto c = pHarm.content(); viewH.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.38f))); c.removeFromTop (6);
      grid (c, { kHcentre.get(), kHwidth.get(), kHspread.get(), kHmix.get() }, 2); }
    { auto c = pCali.content(); grid (c, { kCdrive.get(), kCshape.get(), kCbias.get(), kCjitter.get() }, 2); }
    { auto c = pFilt.content(); segFilt.setBounds (c.removeFromTop (26)); c.removeFromTop (6);
      viewF.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.34f))); c.removeFromTop (6);
      grid (c, { kFcut.get(), kFreso.get(), kFdrive.get(), kFenv2.get(), kFkey.get() }, 3); }

    // ---- ENVELOPES ----
    {
        auto c = pEnv.content();
        auto tabs = c.removeFromTop (22);
        for (int j = 0; j < 3; ++j) envTab[j]->setBounds (tabs.removeFromLeft (56).reduced (1));
        for (auto* dsp : { envDisp[0].get(), envDisp[1].get(), envDisp[2].get() }) dsp->setBounds (c.getX(), c.getY() + 4, c.getWidth(), (int) (c.getHeight() * 0.42f));
        auto faders = c.withTrimmedTop ((int) (c.getHeight() * 0.42f) + 10);
        const int n = 6, cw = faders.getWidth() / n;
        for (int j = 0; j < 3; ++j)
        {
            auto f = faders;
            env[j].a->setBounds (f.removeFromLeft (cw)); env[j].h->setBounds (f.removeFromLeft (cw));
            env[j].d->setBounds (f.removeFromLeft (cw)); env[j].s->setBounds (f.removeFromLeft (cw));
            env[j].r->setBounds (f.removeFromLeft (cw)); env[j].velo->setBounds (f);
        }
    }

    // ---- LFO ----
    {
        auto c = pLfo.content();
        auto tabs = c.removeFromTop (22);
        for (int j = 0; j < 3; ++j) lfoTab[j]->setBounds (tabs.removeFromLeft (56).reduced (1));
        auto dispArea = juce::Rectangle<int> (c.getX(), c.getY() + 4, c.getWidth(), (int) (c.getHeight() * 0.34f));
        for (auto* d : { lfoDisp[0].get(), lfoDisp[1].get(), lfoDisp[2].get() }) d->setBounds (dispArea);
        auto rest = c.withTrimmedTop ((int) (c.getHeight() * 0.34f) + 8);
        auto ctl = rest.removeFromTop (40);
        rest.removeFromTop (4);
        for (int j = 0; j < 3; ++j)
        {
            auto cc = ctl;
            lfo[j].shape->setBounds (cc.removeFromLeft ((int) (cc.getWidth() * 0.44f)).reduced (2));
            lfo[j].sync->setBounds (cc.removeFromLeft (cc.getWidth() / 2).reduced (2));
            lfo[j].retrig->setBounds (cc.reduced (2));
            grid (rest, { lfo[j].rate.get(), lfo[j].smooth.get(), lfo[j].morph.get(), lfo[j].rise.get() }, 4);
        }
    }

    // ---- VOICE ----
    {
        auto c = pVoice.content();
        auto top = c.removeFromTop (26);
        mpe.setBounds (top.removeFromRight (56));
        segMode.setBounds (top.reduced (0, 0));
        c.removeFromTop (8);
        grid (c, { kVoices.get(), kBend.get(), kTune.get(), kGlide.get(), kDrift.get() }, 3);
    }

    // ---- FILTER 2 ----
    {
        auto c = pF2.content();
        segF2.setBounds (c.removeFromTop (26)); c.removeFromTop (6);
        vowelPad.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.3f))); c.removeFromTop (6);
        segRoute.setBounds (c.removeFromTop (22)); c.removeFromTop (6);
        grid (c, { kF2reso.get(), kF2mix.get(), kF2orbit.get(), kF2speed.get() }, 4);
    }
}
