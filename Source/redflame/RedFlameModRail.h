#pragma once
#include "RedFlameWidgets.h"

namespace rf
{
    // A single modulation-source chip. Coloured by family; a small glyph hints the
    // source type. (Drag-to-assign onto knobs is a later pass; this is the styled rail.)
    class Chip : public juce::Component
    {
    public:
        Chip (juce::String labelText, int familyIndex, int sourceIdx)
            : label (std::move (labelText)), family (familyIndex),
              colour (rf::modFamily (familyIndex)), source (sourceIdx)
        {
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragging) return;
            if (e.getDistanceFromDragStart() < 6) return;
            if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
            {
                dragging = true;
                c->startDragging ("modsrc:" + juce::String (source), this,
                                  juce::ScaledImage (createComponentSnapshot (getLocalBounds())), true);
            }
        }
        void mouseUp (const juce::MouseEvent&) override { dragging = false; }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.5f);
            const float rad = r.getHeight() * 0.5f;
            g.setColour (colour.withAlpha (hover ? 0.22f : 0.14f));
            g.fillRoundedRectangle (r, rad);
            g.setColour (colour.withAlpha (hover ? 1.0f : 0.8f));
            g.drawRoundedRectangle (r, rad, 1.2f);

            auto glyph = r.removeFromLeft (r.getHeight()).reduced (6.0f);
            drawGlyph (g, glyph);

            g.setColour (rf::text);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawFittedText (label.toUpperCase(), r.toNearestInt().reduced (2, 0),
                              juce::Justification::centredLeft, 1, 0.8f);
        }

        void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

    private:
        void drawGlyph (juce::Graphics& g, juce::Rectangle<float> a)
        {
            g.setColour (colour);
            juce::Path p;
            switch (family)
            {
                case 0: // envelope: a ramp-up / ramp-down
                    p.startNewSubPath (a.getX(), a.getBottom());
                    p.lineTo (a.getX() + a.getWidth() * 0.35f, a.getY());
                    p.lineTo (a.getRight(), a.getBottom());
                    break;
                case 1: // LFO: a sine
                {
                    const int n = 16;
                    for (int i = 0; i <= n; ++i)
                    {
                        const float t = (float) i / n;
                        const float yy = a.getCentreY() - std::sin (t * juce::MathConstants<float>::twoPi) * a.getHeight() * 0.45f;
                        if (i == 0) p.startNewSubPath (a.getX() + t * a.getWidth(), yy);
                        else        p.lineTo         (a.getX() + t * a.getWidth(), yy);
                    }
                    break;
                }
                case 2: // macro: a horizontal slider dot
                    p.startNewSubPath (a.getX(), a.getCentreY());
                    p.lineTo (a.getRight(), a.getCentreY());
                    g.strokePath (p, juce::PathStrokeType (1.4f));
                    g.fillEllipse (a.getCentreX() - 2.0f, a.getCentreY() - 2.0f, 4.0f, 4.0f);
                    return;
                default: // midi: little bars (velocity)
                    for (int i = 0; i < 3; ++i)
                    {
                        auto bar = a.removeFromLeft (a.getWidth() / (3 - i)).reduced (0.8f, 0.0f);
                        g.fillRect (bar.withTop (bar.getY() + bar.getHeight() * (0.6f - i * 0.25f)));
                    }
                    return;
            }
            g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        juce::String label;
        int family;
        juce::Colour colour;
        int source;
        bool hover = false;
        bool dragging = false;
    };

    // The full rail. Pass a list of {label, family} in display order; it groups them
    // with small gaps between families.
    class ModRail : public juce::Component
    {
    public:
        struct Entry { juce::String label; int family; };

        explicit ModRail (const std::vector<Entry>& entries)
        {
            for (int i = 0; i < (int) entries.size(); ++i)
                chips.add (new Chip (entries[(size_t) i].label, entries[(size_t) i].family, i));
            for (auto* c : chips) addAndMakeVisible (c);
            families.reserve (entries.size());
            for (auto& e : entries) families.push_back (e.family);
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (rf::bg);
            g.fillRect (getLocalBounds());
            g.setColour (rf::hairline);
            g.fillRect (getLocalBounds().removeFromTop (1));
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (10, 7);
            const int n = chips.size();
            if (n == 0) return;

            // count family-change gaps
            int gaps = 0;
            for (int i = 1; i < n; ++i) if (families[(size_t) i] != families[(size_t) i - 1]) ++gaps;
            const int gapW = 14;
            const int avail = r.getWidth() - gaps * gapW;
            const int chipW = juce::jmax (40, avail / n);

            for (int i = 0; i < n; ++i)
            {
                if (i > 0 && families[(size_t) i] != families[(size_t) i - 1]) r.removeFromLeft (gapW);
                chips[i]->setBounds (r.removeFromLeft (i == n - 1 ? r.getWidth() : chipW).reduced (2, 0));
            }
        }

    private:
        juce::OwnedArray<Chip> chips;
        std::vector<int> families;
    };
}
