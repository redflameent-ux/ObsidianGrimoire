#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RedFlameTheme.h"
#include "RedFlameMod.h"
#include "RedFlamePresets.h"

namespace rf
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // ------------------------------------------------------------------ panel "card"
    // A styled container: dark rounded card, accent header tick + uppercase title.
    // Add child controls to it and lay them out inside content().
    class Panel : public juce::Component
    {
    public:
        Panel (juce::String titleText, juce::Colour accentColour = rf::accOracle)
            : title (std::move (titleText)), accent (accentColour) {}

        void setAccent (juce::Colour c) { accent = c; repaint(); }

        // inner area below the header strip, inset by the card padding
        juce::Rectangle<int> content() const
        {
            return getLocalBounds().reduced (m::cardPad)
                     .withTrimmedTop (m::headerH - m::cardPad + 4);
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (rf::panel);
            g.fillRoundedRectangle (r, m::cardRadius);
            g.setColour (rf::border);
            g.drawRoundedRectangle (r, m::cardRadius, 1.0f);

            // header: short accent tick + label
            auto head = getLocalBounds().withHeight (m::headerH).reduced (m::cardPad, 0).translated (0, 6);
            auto tick = head.removeFromLeft (16).withSizeKeepingCentre (14, 3);
            g.setColour (accent);
            g.fillRoundedRectangle (tick.toFloat(), 1.5f);
            head.removeFromLeft (8);
            g.setColour (rf::textDim);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (title.toUpperCase(), head, juce::Justification::centredLeft);
        }

    private:
        juce::String title;
        juce::Colour accent;
    };

    // ------------------------------------------------------------------ metal value knob
    // Doubles as a modulation drop target: drag a rail chip onto it to assign a source,
    // which appears as a coloured dot on the arc (drag the dot = depth, right-click = remove).
    class Knob : public juce::Component,
                 public juce::DragAndDropTarget
    {
    public:
        Knob (APVTS& state, const juce::String& id, juce::Colour accent,
              juce::String labelText = {}, ModMatrix* modMatrix = nullptr)
            : destID (id), matrix (modMatrix)
        {
            auto* prm = state.getParameter (id);
            jassert (prm != nullptr);
            name  = labelText.isNotEmpty() ? labelText : prm->getName (16);
            param = prm;

            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
            slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                        juce::MathConstants<float>::pi * 2.75f, true);
            slider.setTooltip (prm->getName (64));
            slider.onValueChange = [this] { layoutDots(); repaint(); };
            addAndMakeVisible (slider);
            att = std::make_unique<APVTS::SliderAttachment> (state, id, slider);
        }

        // ---- mod dots ----
        void rebuildDots()
        {
            dots.clear();
            if (matrix == nullptr) return;
            for (int src : matrix->sourcesFor (destID))
            {
                auto* d = dots.add (new ModDot (*matrix, src, destID,
                    [this] { layoutDots(); repaint(); },
                    [this, src] { if (matrix) matrix->remove (src, destID); }));
                addAndMakeVisible (d);
            }
            layoutDots();
            repaint();
        }

        // ---- DragAndDropTarget ----
        bool isInterestedInDragSource (const SourceDetails& s) override
        {
            return matrix != nullptr && s.description.toString().startsWith ("modsrc:");
        }
        void itemDragEnter (const SourceDetails&) override { dragOver = true;  repaint(); }
        void itemDragExit  (const SourceDetails&) override { dragOver = false; repaint(); }
        void itemDropped (const SourceDetails& s) override
        {
            dragOver = false;
            if (matrix == nullptr) return;
            const int src = s.description.toString().fromFirstOccurrenceOf ("modsrc:", false, false).getIntValue();
            matrix->set (src, destID, 0.30f);   // fires onChange -> editor rebuilds all dots
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds();
            auto txt = r.removeFromBottom (m::knobLabelH + m::knobValueH);
            g.setColour (rf::text);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawFittedText (name.toUpperCase(), txt.removeFromTop (m::knobLabelH),
                              juce::Justification::centred, 1, 0.8f);
            g.setColour (rf::textDim);
            g.setFont (juce::FontOptions (11.0f));
            g.drawFittedText (param->getCurrentValueAsText(), txt, juce::Justification::centredTop, 1, 0.8f);

            if (dragOver)
            {
                g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
                g.drawRoundedRectangle (slider.getBounds().toFloat().reduced (1.0f), 6.0f, 1.6f);
            }
        }

        void resized() override
        {
            auto r = getLocalBounds();
            r.removeFromBottom (m::knobLabelH + m::knobValueH);
            const int s = juce::jmin (r.getWidth(), r.getHeight());
            slider.setBounds (r.withSizeKeepingCentre (s, s));
            layoutDots();
        }

        juce::Slider slider;

    private:
        void layoutDots()
        {
            if (dots.isEmpty()) return;
            const auto sb = slider.getBounds().toFloat();
            const auto c  = sb.getCentre();
            const float R = juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f;
            const float arcR = R - 2.0f;
            const float start = juce::MathConstants<float>::pi * 1.25f;
            const float end   = juce::MathConstants<float>::pi * 2.75f;
            const float span  = end - start;
            const float baseNorm = (float) slider.valueToProportionOfLength (slider.getValue());
            const float baseAng  = start + baseNorm * span;

            for (auto* d : dots)
            {
                const float depth = matrix != nullptr ? matrix->depth (d->source, d->destID) : 0.0f;
                const float ang = juce::jlimit (start, end, baseAng + depth * span * 0.5f);
                const float x = c.x + std::sin (ang) * arcR;
                const float y = c.y - std::cos (ang) * arcR;
                d->setBounds (juce::Rectangle<float> (x - 5.0f, y - 5.0f, 10.0f, 10.0f).toNearestInt());
            }
        }

        juce::String name, destID;
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<APVTS::SliderAttachment> att;
        ModMatrix* matrix = nullptr;
        juce::OwnedArray<ModDot> dots;
        bool dragOver = false;
    };

    // ------------------------------------------------------------------ vertical fader + caption/value
    class VSlider : public juce::Component
    {
    public:
        VSlider (APVTS& state, const juce::String& id, juce::Colour accent, juce::String labelText = {})
        {
            auto* prm = state.getParameter (id);
            name  = labelText.isNotEmpty() ? labelText : prm->getName (8);
            param = prm;
            slider.setSliderStyle (juce::Slider::LinearVertical);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setColour (juce::Slider::trackColourId, accent);
            slider.setColour (juce::Slider::backgroundColourId, rf::panelLo);
            slider.setColour (juce::Slider::thumbColourId, rf::text);
            slider.setTooltip (prm->getName (64));
            slider.onValueChange = [this] { repaint(); };
            addAndMakeVisible (slider);
            att = std::make_unique<APVTS::SliderAttachment> (state, id, slider);
        }
        void paint (juce::Graphics& g) override
        {
            auto txt = getLocalBounds().removeFromBottom (m::knobLabelH + m::knobValueH);
            g.setColour (rf::text);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawFittedText (name.toUpperCase(), txt.removeFromTop (m::knobLabelH), juce::Justification::centred, 1, 0.8f);
            g.setColour (rf::textDim);
            g.setFont (juce::FontOptions (10.0f));
            g.drawFittedText (param->getCurrentValueAsText(), txt, juce::Justification::centredTop, 1, 0.8f);
        }
        void resized() override
        {
            auto r = getLocalBounds(); r.removeFromBottom (m::knobLabelH + m::knobValueH);
            slider.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 22), r.getHeight()));
        }
        juce::Slider slider;
    private:
        juce::String name;
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };

    // ------------------------------------------------------------------ pill toggle + caption
    class Toggle : public juce::Component
    {
    public:
        Toggle (APVTS& state, const juce::String& id, juce::Colour accent, juce::String labelText = {})
        {
            auto* prm = state.getParameter (id);
            name = labelText.isNotEmpty() ? labelText : prm->getName (16);
            button.setColour (juce::ToggleButton::tickColourId, accent);
            button.setTooltip (prm->getName (64));
            addAndMakeVisible (button);
            att = std::make_unique<APVTS::ButtonAttachment> (state, id, button);
        }
        void paint (juce::Graphics& g) override
        {
            g.setColour (rf::text);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawFittedText (name.toUpperCase(), getLocalBounds().removeFromBottom (m::knobLabelH),
                              juce::Justification::centred, 1, 0.8f);
        }
        void resized() override
        {
            auto r = getLocalBounds(); r.removeFromBottom (m::knobLabelH);
            button.setBounds (r.withSizeKeepingCentre (34, 20));
        }
        juce::ToggleButton button;
    private:
        juce::String name;
        std::unique_ptr<APVTS::ButtonAttachment> att;
    };

    // ------------------------------------------------------------------ dropdown + caption
    class Choice : public juce::Component
    {
    public:
        Choice (APVTS& state, const juce::String& id, juce::String labelText = {})
        {
            auto* prm = state.getParameter (id);
            name = labelText.isNotEmpty() ? labelText : prm->getName (16);
            box.addItemList (prm->getAllValueStrings(), 1);
            box.setTooltip (prm->getName (64));
            addAndMakeVisible (box);
            att = std::make_unique<APVTS::ComboBoxAttachment> (state, id, box);
        }
        void paint (juce::Graphics& g) override
        {
            g.setColour (rf::text);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawFittedText (name.toUpperCase(), getLocalBounds().removeFromBottom (m::knobLabelH),
                              juce::Justification::centred, 1, 0.8f);
        }
        void resized() override
        {
            auto r = getLocalBounds(); r.removeFromBottom (m::knobLabelH);
            box.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4, 104), 24));
        }
        juce::ComboBox box;
    private:
        juce::String name;
        std::unique_ptr<APVTS::ComboBoxAttachment> att;
    };

    // ------------------------------------------------------------------ segmented selector
    // A row of buttons bound to a choice/int parameter (e.g. LP24 | LP12 | BP12 | HP12).
    class Segmented : public juce::Component,
                      private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        Segmented (APVTS& s, const juce::String& id, juce::Colour accentColour,
                   juce::StringArray labels = {})
            : state (s), paramID (id), accent (accentColour)
        {
            param = state.getParameter (id);
            jassert (param != nullptr);
            if (labels.isEmpty()) labels = param->getAllValueStrings();
            numItems = labels.size();

            for (int i = 0; i < numItems; ++i)
            {
                auto* b = buttons.add (new juce::TextButton (labels[i]));
                b->setClickingTogglesState (false);
                b->setColour (juce::TextButton::textColourOffId, rf::textDim);
                b->onClick = [this, i] { select (i); };
                addAndMakeVisible (b);
            }
            state.addParameterListener (paramID, this);
            refresh();
        }

        ~Segmented() override { state.removeParameterListener (paramID, this); }

        void resized() override
        {
            auto r = getLocalBounds();
            const int n = juce::jmax (1, numItems);
            const int w = r.getWidth() / n;
            for (int i = 0; i < numItems; ++i)
                buttons[i]->setBounds (r.removeFromLeft (i == numItems - 1 ? r.getWidth() : w).reduced (1));
        }

    private:
        void select (int i)
        {
            if (param == nullptr || numItems < 2) return;
            param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
        }
        void refresh()
        {
            const int cur = param != nullptr
                ? (int) std::round (param->convertFrom0to1 (param->getValue()) ) : 0;
            for (int i = 0; i < numItems; ++i)
                buttons[i]->setToggleState (i == cur, juce::dontSendNotification);
        }
        void parameterChanged (const juce::String&, float) override
        {
            juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<Segmented> (this)]
                                             { if (sp != nullptr) sp->refresh(); });
        }

        APVTS& state;
        juce::String paramID;
        juce::Colour accent;
        juce::RangedAudioParameter* param = nullptr;
        int numItems = 0;
        juce::OwnedArray<juce::TextButton> buttons;
    };
}
