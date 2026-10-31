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

TormentEditor::TormentEditor (TormentProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "TORMENT", acc, { "MAIN", "SAMPLE" }, tid::master),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);
    for (auto* pnl : { &pAge, &pScope, &pCapture, &pTape, &pDecimate, &pVinyl, &pGrain }) addAndMakeVisible (pnl);

    // AGE
    kAge = knob (pAge, tid::age, "Age"); kMix = knob (pAge, tid::mix, "Mix"); kPunch = knob (pAge, tid::punch, "Punch");
    kTape = knob (pAge, tid::tapeAmt, "Tape"); kDec = knob (pAge, tid::decAmt, "Decimate"); kVin = knob (pAge, tid::vinAmt, "Vinyl");

    // SCOPE
    pScope.addAndMakeVisible (crt);

    // CAPTURE
    pCapture.addAndMakeVisible (segCapLen); pCapture.addAndMakeVisible (segCapMode);
    kBpm = knob (pCapture, tid::bpm, "BPM");
    for (auto* bt : { &recBtn, &grabBtn, &wavBtn }) pCapture.addAndMakeVisible (bt);

    // TAPE
    kTDrive = knob (pTape, tid::tDrive, "Drive"); kTBump = knob (pTape, tid::tBump, "Bump");
    kTFlutter = knob (pTape, tid::tFlutter, "Flutter"); kTHiss = knob (pTape, tid::tHiss, "Hiss");

    // DECIMATE
    pDecimate.addAndMakeVisible (chMachine); pDecimate.addAndMakeVisible (tgUlaw);
    kDRate = knob (pDecimate, tid::dRate, "Rate"); kDBits = knob (pDecimate, tid::dBits, "Bits");
    kDSmooth = knob (pDecimate, tid::dSmooth, "Smooth"); kDJitter = knob (pDecimate, tid::dJitter, "Jitter");

    // VINYL
    pVinyl.addAndMakeVisible (segSpeed);
    kVDust = knob (pVinyl, tid::vDust, "Dust"); kVPops = knob (pVinyl, tid::vPops, "Pops"); kVRumble = knob (pVinyl, tid::vRumble, "Rumble");
    kVWow = knob (pVinyl, tid::vWow, "Wow"); kVTone = knob (pVinyl, tid::vTone, "Tone"); kVMono = knob (pVinyl, tid::vMono, "Mono");

    // GRAIN
    pGrain.addAndMakeVisible (tgFreeze); pGrain.addAndMakeVisible (chShape); pGrain.addAndMakeVisible (chScale);
    pGrain.addAndMakeVisible (chSync); pGrain.addAndMakeVisible (segPitchMode);
    kGrain = knob (pGrain, tid::gGrain, "Grain"); kGSize = knob (pGrain, tid::gSize, "Size"); kGDensity = knob (pGrain, tid::gDensity, "Density");
    kGSpray = knob (pGrain, tid::gSpray, "Spray"); kGDetune = knob (pGrain, tid::gDetune, "Detune"); kGWidth = knob (pGrain, tid::gWidth, "Width");
    kGReverse = knob (pGrain, tid::gReverse, "Reverse"); kGPitch = knob (pGrain, tid::gPitch, "Pitch"); kGWarp = knob (pGrain, tid::gWarp, "Warp");
    kGPositn = knob (pGrain, tid::gPositn, "Positn"); kGFdbk = knob (pGrain, tid::gFdbk, "Fdbk"); kGChance = knob (pGrain, tid::gChance, "Chance");
    kGOctave = knob (pGrain, tid::gOctave, "Octave"); kGJitter = knob (pGrain, tid::gJitter, "Jitter"); kGColor = knob (pGrain, tid::gColor, "Color");
    kGReso = knob (pGrain, tid::gReso, "Reso");

    addAndMakeVisible (modRail);
    topBar.onTab = [] (int) {};

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<TormentEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    setSize (1480, 900);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1240, 800, 2400, 1500);
}

TormentEditor::~TormentEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> TormentEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void TormentEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void TormentEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    g.setColour (rf::textDim); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Welcome to TORMENT. Hover over anything and this bar explains it. Drag a chip onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("HI-FI LO-FI PROCESSOR  v0.1.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void TormentEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));
    modRail.setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (24);

    auto content = r.reduced (10, 8);
    const int g = 8, H = content.getHeight();
    auto row1 = content.removeFromTop ((int) ((H - 2 * g) * 0.31f)); content.removeFromTop (g);
    auto row2 = content.removeFromTop ((int) ((H - 2 * g) * 0.29f)); content.removeFromTop (g);
    auto row3 = content;

    layoutPanels (row1, { &pAge, &pScope, &pCapture }, { 1.0f, 1.5f, 1.0f }, 10);
    layoutPanels (row2, { &pTape, &pDecimate, &pVinyl }, { 1.0f, 1.2f, 1.5f }, 10);
    pGrain.setBounds (row3);

    grid (pAge.content(), { kAge.get(), kMix.get(), kPunch.get(), kTape.get(), kDec.get(), kVin.get() }, 3);
    crt.setBounds (pScope.content());

    {
        auto c = pCapture.content();
        segCapLen.setBounds (c.removeFromTop (24)); c.removeFromTop (6);
        auto rowc = c.removeFromTop (46);
        segCapMode.setBounds (rowc.removeFromLeft (rowc.getWidth() / 2).withSizeKeepingCentre (rowc.getWidth() / 2 - 4, 24));
        kBpm->setBounds (rowc);
        auto rowb = c.removeFromBottom (24);
        const int bw = rowb.getWidth() / 3;
        recBtn.setBounds (rowb.removeFromLeft (bw).reduced (2)); grabBtn.setBounds (rowb.removeFromLeft (bw).reduced (2)); wavBtn.setBounds (rowb.reduced (2));
    }

    grid (pTape.content(), { kTDrive.get(), kTBump.get(), kTFlutter.get(), kTHiss.get() }, 4);
    {
        auto c = pDecimate.content();
        auto top = c.removeFromTop (26); chMachine.setBounds (top.removeFromLeft (150)); tgUlaw.setBounds (top.removeFromRight (70));
        c.removeFromTop (4);
        grid (c, { kDRate.get(), kDBits.get(), kDSmooth.get(), kDJitter.get() }, 4);
    }
    {
        auto c = pVinyl.content();
        segSpeed.setBounds (c.removeFromTop (24).withSizeKeepingCentre (160, 24)); c.removeFromTop (4);
        grid (c, { kVDust.get(), kVPops.get(), kVRumble.get(), kVWow.get(), kVTone.get(), kVMono.get() }, 6);
    }
    {
        auto c = pGrain.content();
        auto top = c.removeFromTop (26);
        tgFreeze.setBounds (top.removeFromLeft (76));
        chShape.setBounds (top.removeFromLeft (130)); chScale.setBounds (top.removeFromLeft (130)); chSync.setBounds (top.removeFromLeft (130));
        segPitchMode.setBounds (top.removeFromRight (120).withSizeKeepingCentre (120, 24));
        c.removeFromTop (6);
        grid (c, { kGrain.get(), kGSize.get(), kGDensity.get(), kGSpray.get(), kGDetune.get(), kGWidth.get(), kGReverse.get(), kGPitch.get(),
                   kGWarp.get(), kGPositn.get(), kGFdbk.get(), kGChance.get(), kGOctave.get(), kGJitter.get(), kGColor.get(), kGReso.get() }, 8);
    }
}
