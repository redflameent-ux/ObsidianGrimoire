#pragma once
#include "RedFlameWidgets.h"

namespace rf
{
    // Top utility bar: brand lockup, plugin wordmark, page tabs, preset browser,
    // SAVE/MIDI/RANDOMIZE/HELP, master knob + output meter.
    class TopBar : public juce::Component
    {
    public:
        TopBar (APVTS& state, juce::String wordmarkText, juce::Colour accentColour,
                juce::StringArray tabNames, const juce::String& masterParamId)
            : wordmark (std::move (wordmarkText)), accent (accentColour)
        {
            for (int i = 0; i < tabNames.size(); ++i)
            {
                auto* b = tabs.add (new juce::TextButton (tabNames[i]));
                b->setClickingTogglesState (false);
                b->onClick = [this, i] { setActiveTab (i); if (onTab) onTab (i); };
                addAndMakeVisible (b);
            }
            if (! tabs.isEmpty()) tabs[0]->setToggleState (true, juce::dontSendNotification);

            help.setButtonText ("HELP");   addAndMakeVisible (help);
            save.setButtonText ("SAVE");   addAndMakeVisible (save);
            midi.setButtonText ("MIDI");   addAndMakeVisible (midi);
            rnd .setButtonText ("RANDOMIZE"); addAndMakeVisible (rnd);
            prevP.setButtonText ("<"); addAndMakeVisible (prevP);
            nextP.setButtonText (">"); addAndMakeVisible (nextP);

            preset.setTextWhenNothingSelected ("Init");
            preset.setJustificationType (juce::Justification::centred);
            preset.addItem ("Init", 1);
            addAndMakeVisible (preset);

            master.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            master.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            master.setColour (juce::Slider::rotarySliderFillColourId, accent);
            master.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                        juce::MathConstants<float>::pi * 2.75f, true);
            addAndMakeVisible (master);
            masterAtt = std::make_unique<APVTS::SliderAttachment> (state, masterParamId, master);
            if (auto* p = state.getParameter (masterParamId)) masterParam = p;
        }

        std::function<void (int)> onTab;

        void setActiveTab (int idx)
        {
            for (int i = 0; i < tabs.size(); ++i)
                tabs[i]->setToggleState (i == idx, juce::dontSendNotification);
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (rf::bg);
            g.fillRect (getLocalBounds());

            // brand mark: three stacked arcs ("signal" waves) in a rounded square
            auto b = brandBox.toFloat();
            g.setColour (rf::panelHi);
            g.fillRoundedRectangle (b, 6.0f);
            g.setColour (rf::border);
            g.drawRoundedRectangle (b, 6.0f, 1.0f);
            {
                auto c = b.reduced (9.0f);
                g.setColour (accent);
                for (int i = 0; i < 3; ++i)
                {
                    juce::Path p;
                    const float yy = c.getY() + c.getHeight() * (0.3f + i * 0.22f);
                    p.startNewSubPath (c.getX(), yy);
                    p.quadraticTo (c.getCentreX(), yy - 6.0f + i * 1.5f, c.getRight(), yy);
                    g.strokePath (p, juce::PathStrokeType (1.6f));
                }
            }
            g.setColour (rf::textDim);
            g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
            g.drawFittedText ("RED FLAME", brandText.removeFromTop (brandText.getHeight() / 2),
                              juce::Justification::centredLeft, 1);
            g.setColour (rf::textFaint);
            g.setFont (juce::FontOptions (6.5f));
            g.drawFittedText ("ENTERTAINMENT", brandText, juce::Justification::centredLeft, 1);

            // plugin wordmark
            g.setColour (accent);
            g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
            g.drawFittedText (wordmark, wordmarkBox, juce::Justification::centredLeft, 1, 0.6f);

            // master readout + meter
            g.setColour (rf::textDim);
            g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
            g.drawFittedText ("MASTER", masterText.removeFromTop (11), juce::Justification::centredRight, 1);
            g.setColour (rf::text);
            g.setFont (juce::FontOptions (10.0f));
            g.drawFittedText (masterParam != nullptr ? masterParam->getCurrentValueAsText() : juce::String ("0.0 dB"),
                              masterText, juce::Justification::centredRight, 1);

            // tiny faux meter bars
            auto mb = meterBox;
            for (int i = 0; i < 8; ++i)
            {
                auto cell = mb.removeFromBottom (mb.getHeight() / (8 - i)).reduced (0, 1).toFloat();
                g.setColour (i < 5 ? accent.withAlpha (0.8f) : rf::border);
                g.fillRect (cell);
            }

            g.setColour (rf::hairline);
            g.fillRect (getLocalBounds().removeFromBottom (1));
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (10, 8);
            brandBox    = r.removeFromLeft (38).withSizeKeepingCentre (38, 38);
            r.removeFromLeft (8);
            brandText   = r.removeFromLeft (64); // painted
            r.removeFromLeft (6);
            wordmarkBox = r.removeFromLeft (188);
            r.removeFromLeft (10);

            // right-hand cluster first
            meterBox   = r.removeFromRight (12).reduced (0, 2);
            r.removeFromRight (8);
            master.setBounds (r.removeFromRight (40).withSizeKeepingCentre (38, 38));
            r.removeFromRight (4);
            masterText = r.removeFromRight (52);
            r.removeFromRight (10);
            nextP.setBounds (r.removeFromRight (26).reduced (1));
            rnd  .setBounds (r.removeFromRight (92).reduced (1));
            prevP.setBounds (r.removeFromRight (26).reduced (1));
            r.removeFromRight (8);
            midi.setBounds (r.removeFromRight (48).reduced (1));
            save.setBounds (r.removeFromRight (48).reduced (1));
            r.removeFromRight (8);

            // tabs (left of centre)
            for (auto* t : tabs) t->setBounds (r.removeFromLeft (62).reduced (1));
            r.removeFromLeft (8);
            help.setBounds (r.removeFromLeft (52).reduced (1));
            r.removeFromLeft (8);

            // preset browser fills the middle gap
            auto pb = r.reduced (4, 2);
            if (pb.getWidth() > 60)
                preset.setBounds (pb);
        }

        juce::TextButton help, save, midi, rnd, prevP, nextP;
        juce::ComboBox   preset;

    private:
        juce::String wordmark;
        juce::Colour accent;
        juce::OwnedArray<juce::TextButton> tabs;
        juce::Slider master;
        std::unique_ptr<APVTS::SliderAttachment> masterAtt;
        juce::RangedAudioParameter* masterParam = nullptr;

        juce::Rectangle<int> brandBox, brandText, wordmarkBox, masterText, meterBox;
    };
}
