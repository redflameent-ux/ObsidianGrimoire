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

EtherealEditor::EtherealEditor (EtherealProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "ETHEREAL", acc, juce::StringArray {}, eid::master),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);
    for (auto* pnl : { &pStack, &pShift, &pSpiral, &pSpace, &pInput, &pOutput, &pPart, &pRumble }) addAndMakeVisible (pnl);

    // STACK
    pStack.addAndMakeVisible (chSlotA); pStack.addAndMakeVisible (chSlotB); pStack.addAndMakeVisible (segRouting);
    kLevelA = knob (pStack, eid::levelA, "Level A"); kLevelB = knob (pStack, eid::levelB, "Level B");

    // GALACTIC SHIFT
    pShift.addAndMakeVisible (segShift);
    kMorph = knob (pShift, eid::morph); kOrbit = knob (pShift, eid::orbit); kReach = knob (pShift, eid::reach);

    // SPIRAL
    pSpiral.addAndMakeVisible (globe); pSpiral.addAndMakeVisible (chPath);
    posSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    posSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    posSlider.setColour (juce::Slider::trackColourId, acc);
    posSlider.setColour (juce::Slider::backgroundColourId, rf::panelLo);
    posSlider.setColour (juce::Slider::thumbColourId, rf::text);
    pSpiral.addAndMakeVisible (posSlider);
    posAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, eid::position, posSlider);
    kSpin = knob (pSpiral, eid::spin); kSpiral = knob (pSpiral, eid::spiral); kHeight = knob (pSpiral, eid::height);

    // SPACE
    pSpace.addAndMakeVisible (tail);
    kSize = knob (pSpace, eid::size); kDecay = knob (pSpace, eid::decay); kMass = knob (pSpace, eid::mass);
    kDamp = knob (pSpace, eid::damp); kDiffuse = knob (pSpace, eid::diffuse); kMod = knob (pSpace, eid::modd, "Mod");
    kDrift = knob (pSpace, eid::drift);

    // INPUT
    kDrive = knob (pInput, eid::drive); kPre = knob (pInput, eid::preDelay, "Pre-Delay");
    kLoCut = knob (pInput, eid::loCut, "Lo Cut"); kHiCut = knob (pInput, eid::hiCut, "Hi Cut");

    // OUTPUT
    pOutput.addAndMakeVisible (tgAuto); pOutput.addAndMakeVisible (tgFreeze);
    kWidth = knob (pOutput, eid::width); kMakeup = knob (pOutput, eid::makeup); kComp = knob (pOutput, eid::comp);
    kEclipse = knob (pOutput, eid::eclipse); kMix = knob (pOutput, eid::mix);

    // PARTIALS
    pPart.addAndMakeVisible (partDisp); pPart.addAndMakeVisible (chPartType);
    kPAmount = knob (pPart, eid::pAmount, "Amount"); kPPitch = knob (pPart, eid::pPitch, "Pitch");
    kPCount = knob (pPart, eid::pCount, "Count"); kPRing = knob (pPart, eid::pRing, "Ring");
    kPShimmer = knob (pPart, eid::pShimmer, "Shimmer"); kPTilt = knob (pPart, eid::pTilt, "Tilt");
    kPSpread = knob (pPart, eid::pSpread, "Spread");

    // RUMBLE
    kRumble = knob (pRumble, eid::rumble); kSub = knob (pRumble, eid::sub); kRDepth = knob (pRumble, eid::rDepth, "Depth");
    kRoll = knob (pRumble, eid::roll); kDark = knob (pRumble, eid::dark);

    // IN/OUT bar
    kInGain = knob (*this, eid::inGain, "Input");
    addAndMakeVisible (analyzer); addAndMakeVisible (segStereo); addAndMakeVisible (chOversample);
    addAndMakeVisible (segQuality); addAndMakeVisible (segHq); addAndMakeVisible (tgLimit);

    addAndMakeVisible (modRail);
    topBar.onTab = [] (int) {};

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<EtherealEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    setSize (1400, 880);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1180, 760, 2400, 1500);
}

EtherealEditor::~EtherealEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> EtherealEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void EtherealEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void EtherealEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    g.setColour (rf::panel);
    g.fillRect (ioBar);
    g.setColour (rf::hairline);
    g.drawHorizontalLine (ioBar.getY(), (float) ioBar.getX(), (float) ioBar.getRight());

    g.setColour (rf::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Welcome to ETHEREAL. Hover over anything and this bar explains it. Drag a chip from the row below onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("GALACTIC SPACE REVERB  v0.1.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void EtherealEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));
    modRail.setBounds (r.removeFromBottom (rf::m::railH));
    ioBar = r.removeFromBottom (60);
    helpStrip = r.removeFromBottom (24);

    // IN/OUT bar contents
    {
        auto io = ioBar.reduced (10, 6);
        kInGain->setBounds (io.removeFromLeft (56)); io.removeFromLeft (8);
        segStereo.setBounds (io.removeFromLeft (150).withSizeKeepingCentre (150, 24)); io.removeFromLeft (10);
        tgLimit.setBounds (io.removeFromRight (64).withSizeKeepingCentre (64, 24));
        segHq.setBounds (io.removeFromRight (86).withSizeKeepingCentre (86, 24)); io.removeFromRight (6);
        segQuality.setBounds (io.removeFromRight (116).withSizeKeepingCentre (116, 24)); io.removeFromRight (6);
        chOversample.setBounds (io.removeFromRight (108)); io.removeFromRight (10);
        analyzer.setBounds (io.reduced (6, 2));
    }

    auto content = r.reduced (10, 8);
    const int g = 8, H = content.getHeight();
    auto r1 = content.removeFromTop ((int) ((H - 2 * g) * 0.44f)); content.removeFromTop (g);
    auto r2 = content.removeFromTop ((int) ((H - 2 * g) * 0.31f)); content.removeFromTop (g);
    auto r3 = content;

    layoutPanels (r1, { &pStack, &pSpiral, &pSpace }, { 1.0f, 1.7f, 1.3f }, 10);
    layoutPanels (r2, { &pShift, &pInput, &pOutput }, { 1.0f, 0.9f, 1.1f }, 10);
    layoutPanels (r3, { &pPart, &pRumble }, { 1.7f, 1.0f }, 10);

    // STACK
    {
        auto c = pStack.content();
        chSlotA.setBounds (c.removeFromTop (38));
        kLevelA->setBounds (c.removeFromTop (64));
        chSlotB.setBounds (c.removeFromTop (38));
        kLevelB->setBounds (c.removeFromTop (64));
        segRouting.setBounds (c.removeFromTop (24));
    }
    // GALACTIC SHIFT
    {
        auto c = pShift.content();
        grid (c.removeFromTop (c.getHeight() - 30), { kMorph.get(), kOrbit.get(), kReach.get() }, 3);
        segShift.setBounds (c.removeFromBottom (24));
    }
    // SPIRAL
    {
        auto c = pSpiral.content();
        globe.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.64f)));
        posSlider.setBounds (c.removeFromTop (22).reduced (4, 2));
        c.removeFromTop (4);
        chPath.setBounds (c.removeFromLeft ((int) (c.getWidth() * 0.26f)).withSizeKeepingCentre ((int) (c.getWidth() * 0.26f) - 4, 44));
        grid (c, { kSpin.get(), kSpiral.get(), kHeight.get() }, 3);
    }
    // SPACE
    {
        auto c = pSpace.content();
        grid (c.removeFromTop (70), { kSize.get(), kDecay.get(), kMass.get() }, 3);
        tail.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.34f))); c.removeFromTop (4);
        grid (c, { kDamp.get(), kDiffuse.get(), kMod.get(), kDrift.get() }, 4);
    }
    // INPUT
    { auto c = pInput.content(); grid (c, { kDrive.get(), kPre.get(), kLoCut.get(), kHiCut.get() }, 2); }
    // OUTPUT
    {
        auto c = pOutput.content();
        grid (c.removeFromTop ((int) (c.getHeight() * 0.56f)), { kWidth.get(), kMakeup.get(), kComp.get(), kEclipse.get() }, 4);
        auto rr = c;
        tgAuto.setBounds (rr.removeFromLeft (92).withSizeKeepingCentre (92, 22));
        tgFreeze.setBounds (rr.removeFromLeft (80).withSizeKeepingCentre (80, 22));
        kMix->setBounds (rr.removeFromRight (70));
    }
    // PARTIALS
    {
        auto c = pPart.content();
        partDisp.setBounds (c.removeFromRight ((int) (c.getWidth() * 0.26f)));
        c.removeFromRight (8);
        chPartType.setBounds (c.removeFromTop (36).removeFromLeft (140));
        grid (c, { kPAmount.get(), kPPitch.get(), kPCount.get(), kPRing.get(),
                   kPShimmer.get(), kPTilt.get(), kPSpread.get() }, 4);
    }
    // RUMBLE
    { auto c = pRumble.content(); grid (c, { kRumble.get(), kSub.get(), kRDepth.get(), kRoll.get(), kDark.get() }, 3); }
}
