#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

// CLEANSE EQ bands. Shared UI<->audio via atomics; serialised to a ValueTree.
namespace cleanse
{
    // type: 0 Bell, 1 LoCut, 2 HiCut, 3 LoShelf, 4 HiShelf
    struct BandInfo { const char* name; int type; float freq; float gain; juce::uint32 colour; };

    static const BandInfo DEFAULTS[8] = {
        { "LO CUT",  1, 30.0f,   0.0f, 0xffe8485f },
        { "LO SHLF", 3, 120.0f, +1.0f, 0xffe8863c },
        { "BELL",    0, 300.0f,  0.0f, 0xff5fe0a0 },
        { "BELL",    0, 800.0f,  0.0f, 0xff8fd04f },
        { "BELL",    0, 2000.0f, 0.0f, 0xff4f86ff },
        { "BELL",    0, 4500.0f, 0.0f, 0xff9b84ff },
        { "BELL",    0, 8000.0f, 0.0f, 0xffe84fb0 },
        { "AIR",     4, 12000.0f,+2.0f,0xff8fe0f0 },
    };

    constexpr int kBands = 8;

    struct Band
    {
        std::atomic<bool>  on { true };
        std::atomic<int>   type { 0 };
        std::atomic<float> freq { 1000.0f }, gain { 0.0f }, q { 0.7f };
    };

    class EqModel
    {
    public:
        EqModel() { reset(); }
        Band bands[kBands];

        void reset()
        {
            for (int i = 0; i < kBands; ++i)
            {
                bands[i].on = (DEFAULTS[i].type != 0) || i < 2 ? true : (i == 2);  // a couple active by default
                bands[i].type = DEFAULTS[i].type; bands[i].freq = DEFAULTS[i].freq;
                bands[i].gain = DEFAULTS[i].gain; bands[i].q = 0.7f;
            }
            bands[0].on = true; bands[1].on = true; bands[7].on = true;  // cut, shelf, air
        }

        juce::ValueTree toTree() const
        {
            juce::ValueTree t ("EQ");
            for (auto& b : bands)
            {
                juce::ValueTree n ("B");
                n.setProperty ("on", b.on.load(), nullptr); n.setProperty ("type", b.type.load(), nullptr);
                n.setProperty ("freq", b.freq.load(), nullptr); n.setProperty ("gain", b.gain.load(), nullptr);
                n.setProperty ("q", b.q.load(), nullptr);
                t.appendChild (n, nullptr);
            }
            return t;
        }
        void fromTree (const juce::ValueTree& t)
        {
            if (! t.hasType ("EQ")) return;
            for (int i = 0; i < juce::jmin (kBands, t.getNumChildren()); ++i)
            {
                auto n = t.getChild (i);
                bands[i].on = (bool) n.getProperty ("on", true); bands[i].type = (int) n.getProperty ("type", 0);
                bands[i].freq = (float) n.getProperty ("freq", 1000.0); bands[i].gain = (float) n.getProperty ("gain", 0.0);
                bands[i].q = (float) n.getProperty ("q", 0.7);
            }
        }
    };
}
