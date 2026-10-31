#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// INCANT - groove generator. Global/perform parameters; the step pattern lives in
// Pattern.h (not the APVTS).
namespace fid
{
    constexpr auto master = "master";
    constexpr auto play   = "play";

    // PERFORM
    constexpr auto energy   = "energy";
    constexpr auto complexr = "complex";
    constexpr auto swing    = "swing";
    constexpr auto thinThk  = "thinthk";

    // GROOVE
    constexpr auto timing   = "timing";
    constexpr auto accent   = "accent";

    // transport / global
    constexpr auto bars  = "bars";
    constexpr auto key   = "key";

    // MACROS
    constexpr auto m1 = "m1", m2 = "m2", m3 = "m3", m4 = "m4";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout incantLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct = [] { return NormalisableRange<float> (0.0f, 100.0f, 0.1f); };
    auto bip = [] { return NormalisableRange<float> (-100.0f, 100.0f, 0.1f); };
    auto& A = L;
    auto f = [&] (const char* id, const char* nm, NormalisableRange<float> r, float d, const char* lbl = "%")
        { A.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d, AudioParameterFloatAttributes().withLabel (lbl))); };
    auto b = [&] (const char* id, const char* nm, bool d) { A.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, d)); };
    auto c = [&] (const char* id, const char* nm, StringArray o, int d) { A.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, o, d)); };

    f (fid::master, "Master", { -60.0f, 12.0f, 0.1f }, -3.0f, "dB");
    b (fid::play, "Play", false);

    f (fid::energy,   "Energy",  pct(), 50.0f);
    f (fid::complexr, "Complex", pct(), 50.0f);
    f (fid::swing,    "Swing",   pct(), 55.0f);
    f (fid::thinThk,  "Thin/Thk", bip(), -30.0f);

    f (fid::timing, "Timing", bip(), 0.0f);
    f (fid::accent, "Accent", pct(), 0.0f);

    c (fid::bars, "Bars", { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2);
    c (fid::key,  "Key",  { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 7);

    f (fid::m1, "Macro 1", pct(), 0.0f); f (fid::m2, "Macro 2", pct(), 0.0f);
    f (fid::m3, "Macro 3", pct(), 0.0f); f (fid::m4, "Macro 4", pct(), 0.0f);

    return L;
}
