#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

// AETHER layer stack: 10 layers, each a (synthesized stand-in for a) sampled sound.
// Shared UI<->audio via atomics; serialised to a ValueTree so it travels with presets.
namespace aether
{
    constexpr int kLayers = 10;

    struct Layer
    {
        std::atomic<bool>  on { false };
        std::atomic<int>   wave { 0 };     // 0 saw 1 square 2 sine 3 tri 4 noise
        std::atomic<float> level { 80.0f }, pan { 0.0f }, coarse { 0.0f }, fine { 0.0f }, root { 60.0f };
        std::atomic<float> start { 0.0f }, loopxf { 10.0f }, drift { 15.0f };
        std::atomic<float> gsize { 80.0f }, gdensity { 3.0f }, gspray { 10.0f }, grandom { 0.0f };
        std::atomic<bool>  loop { true }, reverse { false }, keytrack { true }, autotune { false };
        juce::String name { "empty" };
    };

    class Stack
    {
    public:
        Stack() { reset(); }
        Layer layers[kLayers];

        void reset()
        {
            for (auto& l : layers) { l.on = false; l.name = "empty"; l.wave = 0; l.level = 80.0f; }
            auto init = [this] (int i, const char* nm, int wave, float coarse)
            { layers[i].on = true; layers[i].name = nm; layers[i].wave = wave; layers[i].coarse = coarse; };
            init (0, "Rhodes Tine C2", 2, -12.0f);
            init (1, "Juno Saw Pad",   0, 0.0f);
            init (2, "Vinyl Hat",      4, 0.0f);
            init (3, "FM Bell C3",     3, 0.0f);
        }

        juce::ValueTree toTree() const
        {
            juce::ValueTree t ("STACK");
            for (auto& l : layers)
            {
                juce::ValueTree n ("L");
                n.setProperty ("on", l.on.load(), nullptr);  n.setProperty ("wave", l.wave.load(), nullptr);
                n.setProperty ("name", l.name, nullptr);
                n.setProperty ("level", l.level.load(), nullptr); n.setProperty ("pan", l.pan.load(), nullptr);
                n.setProperty ("coarse", l.coarse.load(), nullptr); n.setProperty ("fine", l.fine.load(), nullptr);
                n.setProperty ("root", l.root.load(), nullptr); n.setProperty ("start", l.start.load(), nullptr);
                n.setProperty ("loopxf", l.loopxf.load(), nullptr); n.setProperty ("drift", l.drift.load(), nullptr);
                n.setProperty ("gsize", l.gsize.load(), nullptr); n.setProperty ("gdensity", l.gdensity.load(), nullptr);
                n.setProperty ("gspray", l.gspray.load(), nullptr); n.setProperty ("grandom", l.grandom.load(), nullptr);
                n.setProperty ("loop", l.loop.load(), nullptr); n.setProperty ("reverse", l.reverse.load(), nullptr);
                n.setProperty ("keytrack", l.keytrack.load(), nullptr); n.setProperty ("autotune", l.autotune.load(), nullptr);
                t.appendChild (n, nullptr);
            }
            return t;
        }
        void fromTree (const juce::ValueTree& t)
        {
            if (! t.hasType ("STACK")) return;
            for (int i = 0; i < juce::jmin (kLayers, t.getNumChildren()); ++i)
            {
                auto n = t.getChild (i); auto& l = layers[i];
                l.on = (bool) n.getProperty ("on"); l.wave = (int) n.getProperty ("wave");
                l.name = n.getProperty ("name", "empty").toString();
                l.level = (float) n.getProperty ("level", 0.8); l.pan = (float) n.getProperty ("pan", 0.0);
                l.coarse = (float) n.getProperty ("coarse", 0.0); l.fine = (float) n.getProperty ("fine", 0.0);
                l.root = (float) n.getProperty ("root", 60.0); l.start = (float) n.getProperty ("start", 0.0);
                l.loopxf = (float) n.getProperty ("loopxf", 10.0); l.drift = (float) n.getProperty ("drift", 15.0);
                l.gsize = (float) n.getProperty ("gsize", 80.0); l.gdensity = (float) n.getProperty ("gdensity", 3.0);
                l.gspray = (float) n.getProperty ("gspray", 10.0); l.grandom = (float) n.getProperty ("grandom", 0.0);
                l.loop = (bool) n.getProperty ("loop", true); l.reverse = (bool) n.getProperty ("reverse", false);
                l.keytrack = (bool) n.getProperty ("keytrack", true); l.autotune = (bool) n.getProperty ("autotune", false);
            }
        }
    };
}
