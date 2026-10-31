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
}

CleanseEditor::CleanseEditor (CleanseProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      topBar (p.apvts, "CLEANSE", acc, juce::StringArray {}, cid::master),
      modRail ({ { "ENV 1", 0 }, { "ENV 2", 0 }, { "ENV 3", 0 },
                 { "LFO 1", 1 }, { "LFO 2", 1 }, { "LFO 3", 1 },
                 { "M1", 2 }, { "M2", 2 }, { "M3", 2 }, { "M4", 2 },
                 { "VELO", 3 }, { "WHEEL", 3 }, { "PRESS", 3 }, { "KEY", 3 }, { "RANDOM", 3 } })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (topBar);

    for (auto* nm : { "FREEZE", "HIGHLIGHT", "FIND CLASHES", "SKETCH", "MATCH" })
    { auto* t = toolBtns.add (new juce::TextButton (nm)); t->setClickingTogglesState (true); addAndMakeVisible (t); }
    addAndMakeVisible (segBands); addAndMakeVisible (segMode);

    curve.onSelect = [this] (int b) { if (b != selBand) selectBand (b); };
    addAndMakeVisible (curve);

    for (int i = 0; i < cleanse::kBands; ++i)
    {
        auto* bt = bandBtns.add (new juce::TextButton (juce::String (i + 1) + " " + cleanse::DEFAULTS[i].name));
        bt->setClickingTogglesState (false);
        bt->onClick = [this, i] { selectBand (i); };
        addAndMakeVisible (bt);
    }

    addAndMakeVisible (pBand); addAndMakeVisible (pOut);
    bandTitle.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    bandTitle.setColour (juce::Label::textColourId, rf::text);
    pBand.addAndMakeVisible (bandTitle);
    auto setupS = [this] (juce::Slider& s) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 15);
        s.setColour (juce::Slider::rotarySliderFillColourId, acc);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        pBand.addAndMakeVisible (s);
    };
    setupS (freqSlider); setupS (gainSlider); setupS (qSlider);
    freqSlider.setRange (20.0, 20000.0, 1.0); freqSlider.setSkewFactorFromMidPoint (1000.0);
    gainSlider.setRange (-15.0, 15.0, 0.1);
    qSlider.setRange (0.1, 10.0, 0.01); qSlider.setSkewFactorFromMidPoint (1.0);
    freqSlider.onValueChange = [this] { proc.eq.bands[selBand].freq.store ((float) freqSlider.getValue()); curve.repaint(); };
    gainSlider.onValueChange = [this] { proc.eq.bands[selBand].gain.store ((float) gainSlider.getValue()); curve.repaint(); };
    qSlider.onValueChange    = [this] { proc.eq.bands[selBand].q.store ((float) qSlider.getValue()); curve.repaint(); };

    typeBox.addItemList ({ "Bell", "Lo Cut", "Hi Cut", "Lo Shelf", "Hi Shelf" }, 1);
    typeBox.onChange = [this] { proc.eq.bands[selBand].type.store (typeBox.getSelectedId() - 1); curve.repaint(); };
    pBand.addAndMakeVisible (typeBox);
    bandOn.setClickingTogglesState (true);
    bandOn.onClick = [this] { proc.eq.bands[selBand].on.store (bandOn.getToggleState()); curve.repaint(); };
    pBand.addAndMakeVisible (bandOn); pBand.addAndMakeVisible (bandSolo);

    kDrive = knob (pOut, cid::drive, "Drive"); kMix = knob (pOut, cid::mix, "Mix");
    kOutput = knob (pOut, cid::output, "Output"); kCeiling = knob (pOut, cid::ceiling, "Ceiling");
    pOut.addAndMakeVisible (chStage); pOut.addAndMakeVisible (chChar);
    pOut.addAndMakeVisible (tgLimiter); pOut.addAndMakeVisible (tgAuto);

    addAndMakeVisible (modRail);
    topBar.onTab = [] (int) {};
    selectBand (selBand);

    proc.modMatrix.onChange = [sp = juce::Component::SafePointer<CleanseEditor> (this)]
    { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->refreshDots(); }); };
    refreshDots();

    setSize (1520, 900);
    setResizable (true, true);
    if (auto* cst = getConstrainer()) cst->setSizeLimits (1260, 800, 2400, 1500);
}

CleanseEditor::~CleanseEditor()
{
    proc.modMatrix.onChange = nullptr;
    setLookAndFeel (nullptr);
}

std::unique_ptr<rf::Knob> CleanseEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob> (proc.apvts, id, acc, label, &proc.modMatrix);
    parent.addAndMakeVisible (*k);
    allKnobs.push_back (k.get());
    return k;
}

void CleanseEditor::refreshDots() { for (auto* k : allKnobs) k->rebuildDots(); }

void CleanseEditor::selectBand (int b)
{
    selBand = b;
    auto& band = proc.eq.bands[b];
    bandTitle.setText ("BAND " + juce::String (b + 1) + "   " + cleanse::DEFAULTS[b].name, juce::dontSendNotification);
    freqSlider.setValue (band.freq.load(), juce::dontSendNotification);
    gainSlider.setValue (band.gain.load(), juce::dontSendNotification);
    qSlider.setValue (band.q.load(), juce::dontSendNotification);
    typeBox.setSelectedId (band.type.load() + 1, juce::dontSendNotification);
    bandOn.setToggleState (band.on.load(), juce::dontSendNotification);
    for (int i = 0; i < bandBtns.size(); ++i) bandBtns[i]->setToggleState (i == b, juce::dontSendNotification);
    curve.repaint();
}

void CleanseEditor::paint (juce::Graphics& g)
{
    g.fillAll (rf::bg);
    // captions for the band sliders
    g.setColour (rf::textDim); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    auto cap = [&] (juce::Slider& s, const char* t)
    { auto b = s.getBounds() + pBand.getPosition(); g.drawText (t, b.getX(), b.getY() - 12, b.getWidth(), 12, juce::Justification::centred); };
    cap (freqSlider, "FREQ"); cap (gainSlider, "GAIN"); cap (qSlider, "Q");

    g.setColour (rf::textDim); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Drag a band point to move it. Click a band to edit it. Drag a chip onto any knob.",
                helpStrip.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (rf::textFaint); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("MASTERING EQ - MIX CLARITY  v0.1.0", helpStrip.reduced (10, 0), juce::Justification::centredRight);
}

void CleanseEditor::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (rf::m::topBarH));

    auto tools = r.removeFromTop (36).reduced (10, 6);
    for (auto* t : toolBtns) t->setBounds (tools.removeFromLeft (t->getButtonText().length() > 6 ? 108 : 78).reduced (1));
    segMode.setBounds (tools.removeFromRight (120).withSizeKeepingCentre (120, 24));
    tools.removeFromRight (8);
    segBands.setBounds (tools.removeFromRight (170).withSizeKeepingCentre (170, 24));

    modRail.setBounds (r.removeFromBottom (rf::m::railH));
    helpStrip = r.removeFromBottom (24);

    auto content = r.reduced (10, 8);
    curve.setBounds (content.removeFromTop ((int) (content.getHeight() * 0.5f)));
    content.removeFromTop (8);
    auto bandRow = content.removeFromTop (28);
    for (auto* bb : bandBtns) bb->setBounds (bandRow.removeFromLeft (bandRow.getWidth() / (bandBtns.size() + 2 - (int) 0)).reduced (1));
    content.removeFromTop (8);

    auto bottom = content;
    pOut.setBounds (bottom.removeFromRight (400)); bottom.removeFromRight (8);
    pBand.setBounds (bottom);

    // band editor
    {
        auto c = pBand.content();
        bandTitle.setBounds (c.removeFromTop (22));
        auto top = c.removeFromTop (26);
        bandOn.setBounds (top.removeFromLeft (54).reduced (1)); bandSolo.setBounds (top.removeFromLeft (54).reduced (1));
        typeBox.setBounds (top.removeFromLeft (110).reduced (1, 1));
        c.removeFromTop (14);  // caption space
        grid (c, { &freqSlider, &gainSlider, &qSlider }, 3);
    }
    // output
    {
        auto c = pOut.content();
        auto top = c.removeFromTop (26); chStage.setBounds (top.removeFromLeft (120)); chChar.setBounds (top.removeFromLeft (130));
        auto tg = c.removeFromBottom (26); tgLimiter.setBounds (tg.removeFromLeft (80)); tgAuto.setBounds (tg.removeFromLeft (70));
        grid (c, { kDrive.get(), kMix.get(), kOutput.get(), kCeiling.get() }, 4);
    }
}
