#include "PluginEditor.h"

namespace
{
    void grid (juce::Rectangle<int> a, std::vector<juce::Component*> cs, int cols, int pad = 3)
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
    const char* kLsel[12] = { aid::lLevel, aid::lPan, aid::lCoarse, aid::lFine, aid::lRoot, aid::lStart,
                              aid::lLoopxf, aid::lDrift, aid::lGsize, aid::lGdensity, aid::lGspray, aid::lGrandom };
}

AetherEditor::AetherEditor (AetherProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "AETHER", acc, { "SYNTH", "MIX", "FX", "MOD", "ARP" }, aid::master),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);
    strip.onSelect = [this] (int n) { loadLayer (n); };
    addAndMakeVisible (strip);

    for (auto* pnl : { &pLayer, &pFiltA, &pFiltB, &pPatina, &pEnv, &pMacros }) addAndMakeVisible (pnl);

    // LAYER
    pLayer.addAndMakeVisible (wave);
    layerName.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    layerName.setColour (juce::Label::textColourId, rf::text);
    pLayer.addAndMakeVisible (layerName);
    const char* tnames[] = { "ON", "LOOP", "REVERSE", "KEY TRACK", "AUTO TUNE" };
    for (int i = 0; i < 5; ++i)
    {
        auto* t = layerToggles.add (new juce::TextButton (tnames[i]));
        t->setClickingTogglesState (true);
        t->onClick = [this, i]
        {
            auto& L = proc.stack.layers[selLayer]; const bool s = layerToggles[i]->getToggleState();
            switch (i) { case 0: L.on.store (s); break; case 1: L.loop.store (s); break; case 2: L.reverse.store (s); break;
                         case 3: L.keytrack.store (s); break; default: L.autotune.store (s); }
            strip.repaint();
        };
        pLayer.addAndMakeVisible (t);
    }
    kLevel = knob (pLayer, aid::lLevel, "Level"); kPan = knob (pLayer, aid::lPan, "Pan");
    kCoarse = knob (pLayer, aid::lCoarse, "Coarse"); kFine = knob (pLayer, aid::lFine, "Fine");
    kRoot = knob (pLayer, aid::lRoot, "Root"); kStart = knob (pLayer, aid::lStart, "Start");
    kLoopxf = knob (pLayer, aid::lLoopxf, "Loop XF"); kDrift = knob (pLayer, aid::lDrift, "Drift");
    kGsize = knob (pLayer, aid::lGsize, "Size"); kGdensity = knob (pLayer, aid::lGdensity, "Density");
    kGspray = knob (pLayer, aid::lGspray, "Spray"); kGrandom = knob (pLayer, aid::lGrandom, "Random");

    // FILTER A / B
    pFiltA.addAndMakeVisible (tgFAon); pFiltA.addAndMakeVisible (chFAtype); pFiltA.addAndMakeVisible (segRoute);
    pFiltB.addAndMakeVisible (tgFBon); pFiltB.addAndMakeVisible (chFBtype);
    kFAcut = knob (pFiltA, aid::fAcut, "Cutoff"); kFAres = knob (pFiltA, aid::fAres, "Res"); kFAdrive = knob (pFiltA, aid::fAdrive, "Drive");
    kFAenv2 = knob (pFiltA, aid::fAenv2, "Env 2"); kFAkey = knob (pFiltA, aid::fAkey, "Key"); kFAmix = knob (pFiltA, aid::fAmix, "Mix");
    kFBcut = knob (pFiltB, aid::fBcut, "Cutoff"); kFBres = knob (pFiltB, aid::fBres, "Res"); kFBdrive = knob (pFiltB, aid::fBdrive, "Drive");
    kFBenv2 = knob (pFiltB, aid::fBenv2, "Env 2"); kFBkey = knob (pFiltB, aid::fBkey, "Key"); kFBmix = knob (pFiltB, aid::fBmix, "Mix");

    // PATINA
    pPatina.addAndMakeVisible (chMachine);
    kPDrift = knob (pPatina, aid::pDrift, "Drift"); kPVariance = knob (pPatina, aid::pVariance, "Variance");
    kPAge = knob (pPatina, aid::pAge, "Age"); kPVca = knob (pPatina, aid::pVca, "VCA");
    kPTape = knob (pPatina, aid::pTape, "Tape"); kPWobble = knob (pPatina, aid::pWobble, "Wobble"); kPNoise = knob (pPatina, aid::pNoise, "Noise");

    // ENV
    pEnv.addAndMakeVisible (envView);
    kEA = knob (pEnv, aid::eA, "A"); kEH = knob (pEnv, aid::eH, "H"); kED = knob (pEnv, aid::eD, "D");
    kES = knob (pEnv, aid::eS, "S"); kER = knob (pEnv, aid::eR, "R"); kCurve = knob (pEnv, aid::eCurve, "Curve"); kVelo = knob (pEnv, aid::eVelo, "Velo");

    // MACROS
    const char* mids[] = { aid::m1, aid::m2, aid::m3, aid::m4, aid::m5, aid::m6, aid::m7, aid::m8 };
    for (int i = 0; i < 8; ++i) kM[i] = knob (pMacros, mids[i], "M" + juce::String (i + 1));

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, rf::panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, rf::panelLo);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, rf::bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, acc.withAlpha (0.8f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, acc.withAlpha (0.35f));
    keyboard.setKeyWidth (20.0f); keyboard.setAvailableRange (24, 96);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (modRail);
    topBar.onTab = [] (int) {};

    for (auto* id : kLsel) proc.apvts.addParameterListener (id, this);
    loadLayer (0);

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<AetherEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    std::vector<rf::PresetManager::Factory> fac;
    auto add = [&fac] (const char* nm, std::function<void()> fn) { fac.push_back ({ nm, std::move (fn) }); };
    add ("Init", [this] { proc.stack.reset();
        rf::setP (proc.apvts, aid::fAcut, 1800); rf::setP (proc.apvts, aid::fAres, 55);
        rf::setP (proc.apvts, aid::eA, 0); rf::setP (proc.apvts, aid::eD, 600); rf::setP (proc.apvts, aid::eS, 100); rf::setP (proc.apvts, aid::eR, 250);
        loadLayer (selLayer); });
    add ("Warm Keys", [this] { auto& s = proc.stack; for (auto& l : s.layers) l.on = false;
        s.layers[0].on = true; s.layers[0].wave = 2; s.layers[0].level = 90; s.layers[0].coarse = 0;
        s.layers[1].on = true; s.layers[1].wave = 3; s.layers[1].level = 55; s.layers[1].coarse = 12;
        rf::setP (proc.apvts, aid::fAcut, 2200); rf::setP (proc.apvts, aid::fAres, 20);
        rf::setP (proc.apvts, aid::eA, 20); rf::setP (proc.apvts, aid::eD, 1200); rf::setP (proc.apvts, aid::eS, 80); rf::setP (proc.apvts, aid::eR, 600); loadLayer (selLayer); });
    add ("Saw Stack", [this] { auto& s = proc.stack; for (auto& l : s.layers) l.on = false;
        s.layers[0].on = true; s.layers[0].wave = 0; s.layers[0].level = 85; s.layers[0].fine = -7;
        s.layers[1].on = true; s.layers[1].wave = 0; s.layers[1].level = 80; s.layers[1].fine = 7;
        s.layers[2].on = true; s.layers[2].wave = 3; s.layers[2].level = 45; s.layers[2].coarse = -12;
        rf::setP (proc.apvts, aid::fAcut, 4500); rf::setP (proc.apvts, aid::fAres, 30);
        rf::setP (proc.apvts, aid::eA, 2); rf::setP (proc.apvts, aid::eD, 800); rf::setP (proc.apvts, aid::eS, 80); rf::setP (proc.apvts, aid::eR, 400); loadLayer (selLayer); });
    add ("Lo Pad", [this] { auto& s = proc.stack; for (auto& l : s.layers) l.on = false;
        s.layers[0].on = true; s.layers[0].wave = 2; s.layers[0].level = 80; s.layers[0].coarse = -12;
        s.layers[1].on = true; s.layers[1].wave = 4; s.layers[1].level = 20;
        rf::setP (proc.apvts, aid::fAcut, 1100); rf::setP (proc.apvts, aid::fAres, 25);
        rf::setP (proc.apvts, aid::eA, 1500); rf::setP (proc.apvts, aid::eD, 2000); rf::setP (proc.apvts, aid::eS, 90); rf::setP (proc.apvts, aid::eR, 2500); loadLayer (selLayer); });
    presetMgr = std::make_unique<rf::PresetManager> (proc, std::move (fac));
    presetMgr->attach (topBar.preset, topBar.prevP, topBar.nextP, topBar.save);

    setSize (1500, 964);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1260, 820, 2400, 1500);
}

AetherEditor::~AetherEditor()
{
    for (auto* id : kLsel) proc.apvts.removeParameterListener (id, this);
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> AetherEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void AetherEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void AetherEditor::loadLayer (int n)
{
    loading = true; selLayer = n;
    auto& L = proc.stack.layers[n];
    auto setP = [this] (const char* id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    setP (aid::lLevel, L.level.load()); setP (aid::lPan, L.pan.load()); setP (aid::lCoarse, L.coarse.load());
    setP (aid::lFine, L.fine.load()); setP (aid::lRoot, L.root.load()); setP (aid::lStart, L.start.load());
    setP (aid::lLoopxf, L.loopxf.load()); setP (aid::lDrift, L.drift.load()); setP (aid::lGsize, L.gsize.load());
    setP (aid::lGdensity, L.gdensity.load()); setP (aid::lGspray, L.gspray.load()); setP (aid::lGrandom, L.grandom.load());
    layerToggles[0]->setToggleState (L.on.load(), juce::dontSendNotification);
    layerToggles[1]->setToggleState (L.loop.load(), juce::dontSendNotification);
    layerToggles[2]->setToggleState (L.reverse.load(), juce::dontSendNotification);
    layerToggles[3]->setToggleState (L.keytrack.load(), juce::dontSendNotification);
    layerToggles[4]->setToggleState (L.autotune.load(), juce::dontSendNotification);
    layerName.setText ("LAYER " + juce::String (n + 1) + "   " + L.name, juce::dontSendNotification);
    strip.selected = n; strip.repaint();
    loading = false;
    for (auto* k : allKnobs) k->repaint();
}

void AetherEditor::parameterChanged (const juce::String& id, float value)
{
    if (loading) return;
    auto& L = proc.stack.layers[selLayer];
    if (id == aid::lLevel) L.level.store (value); else if (id == aid::lPan) L.pan.store (value);
    else if (id == aid::lCoarse) L.coarse.store (value); else if (id == aid::lFine) L.fine.store (value);
    else if (id == aid::lRoot) L.root.store (value); else if (id == aid::lStart) L.start.store (value);
    else if (id == aid::lLoopxf) L.loopxf.store (value); else if (id == aid::lDrift) L.drift.store (value);
    else if (id == aid::lGsize) L.gsize.store (value); else if (id == aid::lGdensity) L.gdensity.store (value);
    else if (id == aid::lGspray) L.gspray.store (value); else if (id == aid::lGrandom) L.grandom.store (value);
}

void AetherEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    g.setColour (rf::textDim); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Drop samples onto the layer cards (or several at once), then play. Drag a chip onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("SAMPLE STACK SYNTHESIZER  v0.2.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void AetherEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));
    strip.setBounds (r.removeFromTop (72).reduced (10, 6));
    keyboard.setBounds (r.removeFromBottom (100));
    modRail.setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (22);

    auto content = r.reduced (10, 8);
    const int g = 8, H = content.getHeight();
    auto row1 = content.removeFromTop ((int) ((H - g) * 0.6f)); content.removeFromTop (g);
    auto row2 = content;

    auto rightCol = row1.removeFromRight (360); row1.removeFromRight (8);
    pLayer.setBounds (row1);
    pFiltA.setBounds (rightCol.removeFromTop ((rightCol.getHeight() - g) / 2)); rightCol.removeFromTop (g);
    pFiltB.setBounds (rightCol);

    pMacros.setBounds (row2.removeFromLeft (330)); row2.removeFromLeft (8);
    pPatina.setBounds (row2.removeFromLeft (340)); row2.removeFromLeft (8);
    pEnv.setBounds (row2);

    // LAYER
    {
        auto c = pLayer.content();
        layerName.setBounds (c.removeFromTop (20));
        wave.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.36f))); c.removeFromTop (4);
        auto tg = c.removeFromTop (24);
        const int tw = tg.getWidth() / layerToggles.size();
        for (auto* t : layerToggles) t->setBounds (tg.removeFromLeft (tw).reduced (1));
        c.removeFromTop (4);
        grid (c.removeFromTop (c.getHeight() / 2), { kLevel.get(), kPan.get(), kCoarse.get(), kFine.get(), kRoot.get(), kStart.get(), kLoopxf.get(), kDrift.get() }, 8);
        grid (c, { kGsize.get(), kGdensity.get(), kGspray.get(), kGrandom.get() }, 8);
    }
    // FILTER A
    {
        auto c = pFiltA.content();
        auto top = c.removeFromTop (26); tgFAon.setBounds (top.removeFromLeft (54)); chFAtype.setBounds (top.removeFromLeft (124));
        segRoute.setBounds (c.removeFromBottom (24)); c.removeFromBottom (4);
        grid (c, { kFAcut.get(), kFAres.get(), kFAdrive.get(), kFAenv2.get(), kFAkey.get(), kFAmix.get() }, 3);
    }
    // FILTER B
    {
        auto c = pFiltB.content();
        auto top = c.removeFromTop (26); tgFBon.setBounds (top.removeFromLeft (54)); chFBtype.setBounds (top.removeFromLeft (124));
        grid (c, { kFBcut.get(), kFBres.get(), kFBdrive.get(), kFBenv2.get(), kFBkey.get(), kFBmix.get() }, 3);
    }
    // PATINA
    {
        auto c = pPatina.content();
        chMachine.setBounds (c.removeFromTop (34).removeFromRight (150));
        grid (c, { kPDrift.get(), kPVariance.get(), kPAge.get(), kPVca.get(), kPTape.get(), kPWobble.get(), kPNoise.get() }, 4);
    }
    // ENV
    {
        auto c = pEnv.content();
        envView.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.44f))); c.removeFromTop (4);
        grid (c, { kEA.get(), kEH.get(), kED.get(), kES.get(), kER.get(), kCurve.get(), kVelo.get() }, 4);
    }
    // MACROS
    grid (pMacros.content(), { kM[0].get(), kM[1].get(), kM[2].get(), kM[3].get(), kM[4].get(), kM[5].get(), kM[6].get(), kM[7].get() }, 4);
}
