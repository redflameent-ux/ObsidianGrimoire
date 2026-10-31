#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RedFlameTheme.h"

namespace rf
{
    // The RedFlame mod rail groups sources the same way on every plugin:
    // 0-2 envelopes, 3-5 LFOs, 6-9 macros, 10+ MIDI (velo/wheel/press/key/random...).
    inline int  familyOfSource (int s) { return s < 3 ? 0 : s < 6 ? 1 : s < 10 ? 2 : 3; }
    inline juce::Colour sourceColour (int s) { return modFamily (familyOfSource (s)); }

    // ---------------------------------------------------------------- mod matrix
    struct ModAssign { int source; juce::String dest; float depth; };

    class ModMatrix
    {
    public:
        std::function<void()> onChange;   // fired on add/remove/load (NOT on live depth edits)

        // notify=false for live depth drags, so open mod-dots aren't torn down mid-gesture
        void set (int src, const juce::String& dest, float depth, bool notify = true)
        {
            for (auto& a : items) if (a.source == src && a.dest == dest) { a.depth = depth; if (notify) fire(); return; }
            items.push_back ({ src, dest, depth });
            if (notify) fire();
        }
        void remove (int src, const juce::String& dest)
        {
            items.erase (std::remove_if (items.begin(), items.end(),
                [&] (const ModAssign& a) { return a.source == src && a.dest == dest; }), items.end());
            fire();
        }
        bool  has   (int src, const juce::String& dest) const { for (auto& a : items) if (a.source == src && a.dest == dest) return true; return false; }
        float depth (int src, const juce::String& dest) const { for (auto& a : items) if (a.source == src && a.dest == dest) return a.depth; return 0.0f; }

        std::vector<int> sourcesFor (const juce::String& dest) const
        {
            std::vector<int> v;
            for (auto& a : items) if (a.dest == dest) v.push_back (a.source);
            return v;
        }

        juce::ValueTree toTree() const
        {
            juce::ValueTree t ("MODMATRIX");
            for (auto& a : items)
            {
                juce::ValueTree n ("A");
                n.setProperty ("src", a.source, nullptr);
                n.setProperty ("dest", a.dest, nullptr);
                n.setProperty ("depth", a.depth, nullptr);
                t.appendChild (n, nullptr);
            }
            return t;
        }
        void fromTree (const juce::ValueTree& t)
        {
            items.clear();
            if (t.hasType ("MODMATRIX"))
                for (int i = 0; i < t.getNumChildren(); ++i)
                {
                    auto n = t.getChild (i);
                    items.push_back ({ (int) n.getProperty ("src"),
                                       n.getProperty ("dest").toString(),
                                       (float) n.getProperty ("depth") });
                }
            fire();
        }

    private:
        void fire() { if (onChange) onChange(); }
        std::vector<ModAssign> items;
    };

    // ---------------------------------------------------------------- draggable mod dot
    // Lives as a child of a Knob, sits on the value arc at the modulated position.
    // Drag up/down = depth; right-click (or alt-click) = remove.
    class ModDot : public juce::Component
    {
    public:
        ModDot (ModMatrix& m, int src, juce::String dest,
                std::function<void()> reposition, std::function<void()> removed)
            : matrix (m), source (src), destID (std::move (dest)),
              colour (sourceColour (src)),
              onReposition (std::move (reposition)), onRemoved (std::move (removed))
        {
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillEllipse (r.translated (0.0f, 0.6f));
            g.setColour (colour.withAlpha (hover ? 1.0f : 0.92f));
            g.fillEllipse (r);
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawEllipse (r, 0.8f);
        }

        void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown() || e.mods.isAltDown()) { if (onRemoved) onRemoved(); return; }
            startDepth = matrix.depth (source, destID);
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) return;
            const float d = startDepth - (float) e.getDistanceFromDragStartY() / 130.0f;
            matrix.set (source, destID, juce::jlimit (-1.0f, 1.0f, d), false);
            if (onReposition) onReposition();
        }

        int source;
        juce::String destID;

    private:
        ModMatrix& matrix;
        juce::Colour colour;
        std::function<void()> onReposition, onRemoved;
        float startDepth = 0.0f;
        bool hover = false;
    };
}
