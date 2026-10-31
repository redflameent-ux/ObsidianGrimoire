#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// CLEANSE - mastering / dynamic EQ (effect, modelled on CLEARING). EQ bands live in
// BandData.h; these are the global / output params.
namespace cid
{
    constexpr auto master = "master";
    constexpr auto drive = "drive", mix = "mix", output = "output", ceiling = "ceiling";
    constexpr auto stage = "stage", character = "character", oversample = "oversample";
    constexpr auto limiter = "limiter", autoGain = "autogain";
    constexpr auto bandCount = "band_count", eqMode = "eq_mode";
    constexpr auto m1 = "m1", m2 = "m2", m3 = "m3", m4 = "m4";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout cleanseLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct = [] { return NormalisableRange<float> (0.0f, 100.0f, 0.1f); };
    auto& A = L;
    auto f = [&] (const char* id, const char* nm, NormalisableRange<float> r, float d, const char* lbl = "%")
        { A.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d, AudioParameterFloatAttributes().withLabel (lbl))); };
    auto b = [&] (const char* id, const char* nm, bool d) { A.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, d)); };
    auto c = [&] (const char* id, const char* nm, StringArray o, int d) { A.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, o, d)); };

    f (cid::master, "Master", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    f (cid::drive, "Drive", pct(), 0.0f); f (cid::mix, "Mix", pct(), 100.0f);
    f (cid::output, "Output", { -24.0f, 12.0f, 0.1f }, 0.0f, "dB"); f (cid::ceiling, "Ceiling", { -12.0f, 0.0f, 0.1f }, -0.5f, "dB");
    c (cid::stage, "Stage", { "Clean", "Tube", "Tape", "FET" }, 1);
    c (cid::character, "Character", { "Clean", "Warm", "Open", "Dark" }, 0);
    c (cid::oversample, "Oversample", { "Off", "2x", "4x" }, 0);
    b (cid::limiter, "Limiter", false); b (cid::autoGain, "Auto", true);
    c (cid::bandCount, "Bands", { "6", "12", "24", "36" }, 1);
    c (cid::eqMode, "Mode", { "EQ", "Spectral" }, 0);
    f (cid::m1, "Macro 1", pct(), 0.0f); f (cid::m2, "Macro 2", pct(), 0.0f);
    f (cid::m3, "Macro 3", pct(), 0.0f); f (cid::m4, "Macro 4", pct(), 0.0f);

    return L;
}
