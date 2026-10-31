#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

// INCANT groove pattern: fixed lanes x 16 steps. Step velocity 0 = off, >0 = hit.
// Shared between the UI (writes) and the audio thread (reads) via atomics, and
// serialised to/from a ValueTree so it travels with the preset.
namespace incant
{
    struct LaneInfo { const char* name; int type; juce::uint32 colour; };

    // 8 lanes, each a synth-drum type + a lane colour for the grid
    static const LaneInfo LANES[8] = {
        { "KICK",  0, 0xffe3a73e },
        { "SNARE", 1, 0xffe8673c },
        { "C-HAT", 2, 0xff35c6e8 },
        { "O-HAT", 3, 0xff4f86ff },
        { "CLAP",  4, 0xffe8485f },
        { "TOM",   5, 0xff9b84ff },
        { "RIM",   6, 0xffe8d23c },
        { "RIDE",  7, 0xff3fb6a8 },
    };

    constexpr int kLanes = 8;
    constexpr int kSteps = 16;

    class Pattern
    {
    public:
        Pattern() { reset(); }

        void reset()
        {
            for (int l = 0; l < kLanes; ++l)
            {
                level[l].store (0.8f); pan[l].store (0.0f); mute[l].store (false);
                for (int s = 0; s < kSteps; ++s) vel[l][s].store (0.0f);
            }
            // a plain house-ish starter groove
            for (int s = 0; s < kSteps; s += 4) vel[0][s].store (0.9f);   // kick 4-on-floor
            vel[1][4].store (0.9f); vel[1][12].store (0.9f);              // snare on 2 & 4
            for (int s = 2; s < kSteps; s += 4) vel[2][s].store (0.6f);  // closed hat offbeats
            vel[3][2].store (0.5f); vel[3][10].store (0.5f);             // open hat
        }

        void clearAll() { for (int l = 0; l < kLanes; ++l) for (int s = 0; s < kSteps; ++s) vel[l][s].store (0.0f); }

        float get (int l, int s) const { return vel[l][s].load(); }
        void  set (int l, int s, float v) { vel[l][s].store (juce::jlimit (0.0f, 1.0f, v)); }
        void  toggle (int l, int s) { set (l, s, get (l, s) > 0.0f ? 0.0f : 0.9f); }

        std::atomic<float> vel[kLanes][kSteps];
        std::atomic<float> level[kLanes];
        std::atomic<float> pan[kLanes];
        std::atomic<bool>  mute[kLanes];

        juce::ValueTree toTree() const
        {
            juce::ValueTree t ("PATTERN");
            for (int l = 0; l < kLanes; ++l)
            {
                juce::ValueTree lane ("L");
                lane.setProperty ("level", level[l].load(), nullptr);
                lane.setProperty ("pan",   pan[l].load(), nullptr);
                lane.setProperty ("mute",  mute[l].load(), nullptr);
                juce::String steps;
                for (int s = 0; s < kSteps; ++s) steps << juce::String (vel[l][s].load(), 2) << (s < kSteps - 1 ? "," : "");
                lane.setProperty ("steps", steps, nullptr);
                t.appendChild (lane, nullptr);
            }
            return t;
        }
        void fromTree (const juce::ValueTree& t)
        {
            if (! t.hasType ("PATTERN")) return;
            for (int l = 0; l < juce::jmin (kLanes, t.getNumChildren()); ++l)
            {
                auto lane = t.getChild (l);
                level[l].store ((float) lane.getProperty ("level", 0.8));
                pan[l].store ((float) lane.getProperty ("pan", 0.0));
                mute[l].store ((bool) lane.getProperty ("mute", false));
                auto parts = juce::StringArray::fromTokens (lane.getProperty ("steps").toString(), ",", "");
                for (int s = 0; s < juce::jmin (kSteps, parts.size()); ++s) vel[l][s].store (parts[s].getFloatValue());
            }
        }
    };
}
