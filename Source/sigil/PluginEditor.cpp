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
    void layoutPanels (juce::Rectangle<int> row, std::vector<rf::Panel*> panels, std::vector<float> w, int gap)
    {
        float sum = 0; for (float x : w) sum += x;
        const int usable = row.getWidth() - gap * ((int) panels.size() - 1);
        for (size_t i = 0; i < panels.size(); ++i)
        {
            const int px = (i == panels.size() - 1) ? row.getWidth() : (int) std::round (usable * (w[i] / sum));
            panels[i]->setBounds (row.removeFromLeft (px));
            if (i < panels.size() - 1) row.removeFromLeft (gap);
        }
    }
}

SigilEditor::SigilEditor (SigilProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "SIGIL", acc, { "SYNTH", "TEXTURE", "ARP" }, sid::master),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);
    addAndMakeVisible (segEngine);

    kM1 = knob (*this, sid::m1, "M1"); kM2 = knob (*this, sid::m2, "M2");
    kM3 = knob (*this, sid::m3, "M3"); kM4 = knob (*this, sid::m4, "M4");

    for (auto* pnl : { &pExc, &pRes, &pSpace, &pMot, &pPlay, &pTouch }) addAndMakeVisible (pnl);

    // EXCITER
    pExc.addAndMakeVisible (segEnv);
    kStrike = knob (pExc, sid::strike); kBow = knob (pExc, sid::bowblow, "Bow/Blow"); kColor = knob (pExc, sid::color);
    kOsc = knob (pExc, sid::osc); kShape = knob (pExc, sid::shape);
    kAtk = knob (pExc, sid::atk, "Attack"); kDec = knob (pExc, sid::dec, "Decay");
    kSus = knob (pExc, sid::sus, "Sustain"); kRel = knob (pExc, sid::rel, "Release");

    // RESONATOR
    pRes.addAndMakeVisible (resDisp);
    pRes.addAndMakeVisible (chChord); pRes.addAndMakeVisible (chKey); pRes.addAndMakeVisible (chScale);
    kStruct = knob (pRes, sid::structure); kBright = knob (pRes, sid::bright, "Brightness");
    kDamp = knob (pRes, sid::damping); kPos = knob (pRes, sid::position);
    kKeyUp = knob (pRes, sid::keyUpDamp, "Key-Up");

    // SPACE
    pSpace.addAndMakeVisible (tgChorus); pSpace.addAndMakeVisible (chChorusType);
    pSpace.addAndMakeVisible (tgReverb); pSpace.addAndMakeVisible (tgSync); pSpace.addAndMakeVisible (chRvType);
    kChAmt = knob (pSpace, sid::chorusAmt, "Amount"); kChDepth = knob (pSpace, sid::chorusDepth, "Depth");
    kChRate = knob (pSpace, sid::chorusRate, "Rate"); kChWidth = knob (pSpace, sid::chorusWidth, "Width");
    kRvMix = knob (pSpace, sid::reverbMix, "Mix"); kRvTime = knob (pSpace, sid::reverbTime, "Time");
    kRvTone = knob (pSpace, sid::reverbTone, "Tone");

    // MOTION
    pMot.addAndMakeVisible (chMotShape);
    kMotRate = knob (pMot, sid::motRate, "Rate"); kMStruct = knob (pMot, sid::motStruct, "Struct");
    kMBright = knob (pMot, sid::motBright, "Bright"); kMPos = knob (pMot, sid::motPos, "Position");
    kMVib = knob (pMot, sid::motVib, "Vibrato"); kMWt = knob (pMot, sid::motWt, "WT");

    // PLAY
    pPlay.addAndMakeVisible (segMode);
    kVoices = knob (pPlay, sid::voices, "Voices"); kGlide = knob (pPlay, sid::glide, "Glide");
    kOctave = knob (pPlay, sid::octave, "Octave"); kTune = knob (pPlay, sid::tune, "Tune");

    // TOUCH
    kVelBrt = knob (pTouch, sid::velBrt, "Vel-Brt"); kVelStk = knob (pTouch, sid::velStk, "Vel-Stk");
    kMwLfo = knob (pTouch, sid::mwLfo, "MW-LFO"); kMwBrt = knob (pTouch, sid::mwBrt, "MW-Brt");
    kAtExc = knob (pTouch, sid::atExc, "AT-Exc"); kBend = knob (pTouch, sid::bend, "Bend");

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

    topBar.onTab = [] (int) {};

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<SigilEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    std::vector<rf::PresetManager::Factory> fac;
    auto add = [&fac] (const char* nm, std::function<void()> fn) { fac.push_back ({ nm, std::move (fn) }); };
    add ("Init", [this] { auto& a = proc.apvts;
        rf::setP (a, sid::structure, 25); rf::setP (a, sid::bright, 50); rf::setP (a, sid::damping, 75); rf::setP (a, sid::position, 48); rf::setP (a, sid::strike, 100);
        rf::setP (a, sid::atk, 1); rf::setP (a, sid::dec, 1000); rf::setP (a, sid::sus, 100); rf::setP (a, sid::rel, 4600); rf::setP (a, sid::reverbOn, 1); rf::setP (a, sid::reverbMix, 25); });
    add ("Glass Mallet", [this] { auto& a = proc.apvts;
        rf::setP (a, sid::structure, 10); rf::setP (a, sid::bright, 80); rf::setP (a, sid::damping, 55); rf::setP (a, sid::position, 30); rf::setP (a, sid::strike, 100);
        rf::setP (a, sid::atk, 1); rf::setP (a, sid::dec, 400); rf::setP (a, sid::sus, 0); rf::setP (a, sid::rel, 1500); });
    add ("Bowed Drone", [this] { auto& a = proc.apvts;
        rf::setP (a, sid::bowblow, 80); rf::setP (a, sid::strike, 20); rf::setP (a, sid::damping, 95); rf::setP (a, sid::bright, 40);
        rf::setP (a, sid::atk, 1500); rf::setP (a, sid::sus, 100); rf::setP (a, sid::rel, 3000); rf::setP (a, sid::reverbMix, 50); });
    add ("Metal Perc", [this] { auto& a = proc.apvts;
        rf::setP (a, sid::structure, 60); rf::setP (a, sid::bright, 90); rf::setP (a, sid::damping, 40); rf::setP (a, sid::position, 70);
        rf::setP (a, sid::dec, 800); rf::setP (a, sid::sus, 0); rf::setP (a, sid::rel, 1200); });
    add ("Deep Space", [this] { auto& a = proc.apvts;
        rf::setP (a, sid::damping, 85); rf::setP (a, sid::bright, 35); rf::setP (a, sid::chorusOn, 1);
        rf::setP (a, sid::reverbOn, 1); rf::setP (a, sid::reverbMix, 60); rf::setP (a, sid::reverbTime, 80); });
    presetMgr = std::make_unique<rf::PresetManager> (proc, std::move (fac));
    presetMgr->attach (topBar.preset, topBar.prevP, topBar.nextP, topBar.save);

    setSize (1400, 900);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1180, 780, 2400, 1500);
}

SigilEditor::~SigilEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> SigilEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void SigilEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void SigilEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    g.setColour (rf::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Welcome to SIGIL. Hover over anything and this bar explains it. Drag a chip from the row below onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("MODAL RESONATOR SYNTHESIZER  v0.1.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void SigilEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));

    // engine strip
    auto eng = r.removeFromTop (62).reduced (10, 8);
    auto macros = eng.removeFromRight (4 * 56);
    grid (macros, { kM1.get(), kM2.get(), kM3.get(), kM4.get() }, 4, 2);
    eng.removeFromRight (10);
    segEngine.setBounds (eng);

    keyboard.setBounds (r.removeFromBottom (rf::m::keyboardH));
    modRail .setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (26);

    auto content = r.reduced (10, 8);
    const int rowGap = 8;
    auto top = content.removeFromTop ((content.getHeight() - rowGap) / 2);
    content.removeFromTop (rowGap);
    auto bot = content;

    layoutPanels (top, { &pExc, &pRes, &pSpace }, { 1.0f, 1.6f, 1.1f }, 10);
    layoutPanels (bot, { &pMot, &pPlay, &pTouch }, { 1.1f, 1.0f, 1.1f }, 10);

    // EXCITER
    {
        auto c = pExc.content();
        grid (c.removeFromTop (76), { kStrike.get(), kBow.get(), kColor.get() }, 3);
        grid (c.removeFromTop (76), { kOsc.get(), kShape.get() }, 2);
        segEnv.setBounds (c.removeFromTop (24).reduced (16, 2)); c.removeFromTop (4);
        grid (c, { kAtk.get(), kDec.get(), kSus.get(), kRel.get() }, 4);
    }
    // RESONATOR
    {
        auto c = pRes.content();
        resDisp.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.32f))); c.removeFromTop (6);
        grid (c.removeFromTop ((int) (c.getHeight() * 0.56f)), { kStruct.get(), kBright.get(), kDamp.get(), kPos.get() }, 4);
        grid (c, { &chChord, &chKey, &chScale, kKeyUp.get() }, 4);
    }
    // SPACE
    {
        auto c = pSpace.content();
        auto r1 = c.removeFromTop (40);
        tgChorus.setBounds (r1.removeFromLeft (78)); chChorusType.setBounds (r1.reduced (4, 2));
        grid (c.removeFromTop (74), { kChAmt.get(), kChDepth.get(), kChRate.get(), kChWidth.get() }, 4);
        auto r2 = c.removeFromTop (40);
        tgReverb.setBounds (r2.removeFromLeft (72)); tgSync.setBounds (r2.removeFromLeft (64)); chRvType.setBounds (r2.reduced (4, 2));
        grid (c, { kRvMix.get(), kRvTime.get(), kRvTone.get() }, 3);
    }
    // MOTION
    {
        auto c = pMot.content();
        chMotShape.setBounds (c.removeFromTop (40).reduced (2)); c.removeFromTop (4);
        grid (c, { kMotRate.get(), kMStruct.get(), kMBright.get(), kMPos.get(), kMVib.get(), kMWt.get() }, 3);
    }
    // PLAY
    {
        auto c = pPlay.content();
        segMode.setBounds (c.removeFromTop (26)); c.removeFromTop (8);
        grid (c, { kVoices.get(), kGlide.get(), kOctave.get(), kTune.get() }, 2);
    }
    // TOUCH
    {
        auto c = pTouch.content();
        grid (c, { kVelBrt.get(), kVelStk.get(), kMwLfo.get(), kMwBrt.get(), kAtExc.get(), kBend.get() }, 3);
    }
}
