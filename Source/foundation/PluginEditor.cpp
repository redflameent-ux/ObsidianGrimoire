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

FoundationEditor::FoundationEditor (FoundationProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "FOUNDATION", acc, { "MAIN", "FORGE" }, fnd::master),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);
    for (auto* pnl : { &pIn, &pSat, &pComp, &pLoud, &pOut }) addAndMakeVisible (pnl);

    // INPUT
    kInput = knob (pIn, fnd::input, "Input");
    pIn.addAndMakeVisible (inLufs); pIn.addAndMakeVisible (segQuality); pIn.addAndMakeVisible (tgLive);

    // SATURATE
    pSat.addAndMakeVisible (spectrum);
    kBass = knob (pSat, fnd::bass, "Bass"); kThick = knob (pSat, fnd::thick, "Thick"); kDepth = knob (pSat, fnd::depth, "Depth");
    kSub = knob (pSat, fnd::sub, "Sub"); kFocus = knob (pSat, fnd::focus, "Focus"); kSpread = knob (pSat, fnd::spread, "Spread");
    kPhantom = knob (pSat, fnd::phantom, "Phantom"); kRoot = knob (pSat, fnd::root, "Root"); kColor = knob (pSat, fnd::color, "Color");

    // COMPRESS
    pComp.addAndMakeVisible (segCompMode); pComp.addAndMakeVisible (tgClip);
    pComp.addAndMakeVisible (reduceMeter); pComp.addAndMakeVisible (limitMeter2);
    kComp = knob (pComp, fnd::comp, "Comp"); kBody = knob (pComp, fnd::body, "Body");
    kSoft = knob (pComp, fnd::soft, "Soft"); kCrunch = knob (pComp, fnd::crunch, "Crunch");

    // LOUD
    pLoud.addAndMakeVisible (limitMeter);
    kLoud = knob (pLoud, fnd::loud, "Loud"); kPunch = knob (pLoud, fnd::punch, "Punch");
    kClarity = knob (pLoud, fnd::clarity, "Clarity"); kShape = knob (pLoud, fnd::shape, "Shape");
    kWidth = knob (pLoud, fnd::width, "Width"); kTarget = knob (pLoud, fnd::target, "Target");

    // OUTPUT
    kMix = knob (pOut, fnd::mix, "Mix"); kOutput = knob (pOut, fnd::output, "Output");
    pOut.addAndMakeVisible (outLufs); pOut.addAndMakeVisible (unityBtn); pOut.addAndMakeVisible (tgBypass);

    addAndMakeVisible (modRail);
    topBar.onTab = [] (int) {};

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<FoundationEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    std::vector<rf::PresetManager::Factory> fac;
    auto add = [&fac] (const char* nm, std::function<void()> fn) { fac.push_back ({ nm, std::move (fn) }); };
    add ("Bus Glue", [this] { auto& a = proc.apvts;
        rf::setP (a, fnd::comp, 35); rf::setP (a, fnd::thick, 25); rf::setP (a, fnd::loud, 20); rf::setP (a, fnd::mix, 100); });
    add ("Loud Master", [this] { auto& a = proc.apvts;
        rf::setP (a, fnd::loud, 60); rf::setP (a, fnd::comp, 40); rf::setP (a, fnd::thick, 30); });
    add ("Warm Saturate", [this] { auto& a = proc.apvts;
        rf::setP (a, fnd::thick, 50); rf::setP (a, fnd::color, 20); rf::setP (a, fnd::comp, 20); });
    presetMgr = std::make_unique<rf::PresetManager> (proc, std::move (fac));
    presetMgr->attach (topBar.preset, topBar.prevP, topBar.nextP, topBar.save);

    setSize (1500, 820);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1240, 720, 2400, 1400);
}

FoundationEditor::~FoundationEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> FoundationEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void FoundationEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void FoundationEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    g.setColour (rf::textDim); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Welcome to FOUNDATION. Hover over anything and this bar explains it. Drag a chip onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("SATURATION + LOUDNESS  v0.1.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void FoundationEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));
    modRail.setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (24);

    layoutPanels (r.reduced (10, 8), { &pIn, &pSat, &pComp, &pLoud, &pOut }, { 0.7f, 1.5f, 1.1f, 1.2f, 0.8f }, 10);

    // INPUT
    {
        auto c = pIn.content();
        kInput->setBounds (c.removeFromTop ((int) (c.getHeight() * 0.42f)));
        inLufs.setBounds (c.removeFromTop (54));
        tgLive.setBounds (c.removeFromBottom (26));
        segQuality.setBounds (c.removeFromBottom (26));
    }
    // SATURATE
    {
        auto c = pSat.content();
        spectrum.setBounds (c.removeFromTop ((int) (c.getHeight() * 0.32f))); c.removeFromTop (4);
        grid (c, { kBass.get(), kThick.get(), kDepth.get(), kSub.get(), kFocus.get(), kSpread.get(), kPhantom.get(), kRoot.get(), kColor.get() }, 3);
    }
    // COMPRESS
    {
        auto c = pComp.content();
        segCompMode.setBounds (c.removeFromTop (24)); c.removeFromTop (4);
        grid (c.removeFromTop ((int) (c.getHeight() * 0.36f)), { kComp.get(), kBody.get() }, 2);
        reduceMeter.setBounds (c.removeFromTop (26).reduced (2, 2));
        tgClip.setBounds (c.removeFromTop (24).removeFromLeft (70));
        limitMeter2.setBounds (c.removeFromBottom (30).reduced (2, 2));
        grid (c, { kSoft.get(), kCrunch.get() }, 2);
    }
    // LOUD
    {
        auto c = pLoud.content();
        grid (c.removeFromTop ((int) (c.getHeight() * 0.36f)), { kLoud.get(), kPunch.get() }, 2);
        limitMeter.setBounds (c.removeFromBottom (30).reduced (2, 2));
        grid (c, { kClarity.get(), kShape.get(), kWidth.get(), kTarget.get() }, 2);
    }
    // OUTPUT
    {
        auto c = pOut.content();
        grid (c.removeFromTop ((int) (c.getHeight() * 0.42f)), { kMix.get(), kOutput.get() }, 1);
        outLufs.setBounds (c.removeFromTop (54));
        tgBypass.setBounds (c.removeFromBottom (26));
        unityBtn.setBounds (c.removeFromBottom (26).reduced (10, 1));
    }
}
