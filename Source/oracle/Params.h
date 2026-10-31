#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// ORACLE parameter ids + APVTS layout.
// This covers the SYNTH page controls the editor renders. Not every parameter drives
// DSP yet (the first pass is the UI kit) - the ones that do are noted in the processor.
namespace pid
{
    // global
    constexpr auto master   = "master";

    // OSC A
    constexpr auto aOn      = "a_on";
    constexpr auto aWt      = "a_wt";
    constexpr auto aFine    = "a_fine";
    constexpr auto aLevel   = "a_level";
    constexpr auto aOct     = "a_oct";
    constexpr auto aSemi    = "a_semi";

    // OSC B
    constexpr auto bOn      = "b_on";
    constexpr auto bWt      = "b_wt";
    constexpr auto bFine    = "b_fine";
    constexpr auto bLevel   = "b_level";
    constexpr auto bNoise   = "b_noise";
    constexpr auto bOct     = "b_oct";
    constexpr auto bSemi    = "b_semi";

    // HARMONICS
    constexpr auto hCentre  = "h_centre";
    constexpr auto hWidth   = "h_width";
    constexpr auto hSpread  = "h_spread";
    constexpr auto hMix     = "h_mix";

    // CALI wavefolder
    constexpr auto cDrive   = "c_drive";
    constexpr auto cShape   = "c_shape";
    constexpr auto cBias    = "c_bias";
    constexpr auto cJitter  = "c_jitter";

    // FILTER
    constexpr auto fType    = "f_type";   // LP24 / LP12 / BP12 / HP12
    constexpr auto fCutoff  = "f_cutoff";
    constexpr auto fReso    = "f_reso";
    constexpr auto fDrive   = "f_drive";
    constexpr auto fEnv2    = "f_env2";
    constexpr auto fKey     = "f_key";

    // VOICE
    constexpr auto vMode    = "v_mode";   // Poly / Mono / Legato
    constexpr auto vVoices  = "v_voices";
    constexpr auto vBend    = "v_bend";
    constexpr auto vTune    = "v_tune";
    constexpr auto vGlide   = "v_glide";
    constexpr auto vDrift   = "v_drift";
    constexpr auto vMpe     = "v_mpe";

    // FILTER 2
    constexpr auto f2Type   = "f2_type";  // Off / Vowel / Comb
    constexpr auto f2Reso   = "f2_reso";
    constexpr auto f2Mix    = "f2_mix";
    constexpr auto f2Orbit  = "f2_orbit";
    constexpr auto f2Speed  = "f2_speed";
    constexpr auto f2Route  = "f2_route"; // Serial / Parallel
}

// env/LFO parameters are generated per index; the editor builds the same ids
inline juce::String envId (int i, const char* s) { return "e" + juce::String (i) + "_" + s; }
inline juce::String lfoId (int i, const char* s) { return "l" + juce::String (i) + "_" + s; }

inline juce::AudioProcessorValueTreeState::ParameterLayout OracleLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct   = [] { return NormalisableRange<float> (0.0f, 100.0f, 0.1f); };
    auto bip   = [] { return NormalisableRange<float> (-100.0f, 100.0f, 0.1f); };
    auto add   = [&L] (auto p) { L.add (std::move (p)); };

    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::master, 1 }, "Master",
            NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f,
            AudioParameterFloatAttributes().withLabel ("dB")));

    // ---- OSC A ----
    add (std::make_unique<AudioParameterBool>   (ParameterID { pid::aOn, 1 }, "Osc A", true));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::aWt, 1 }, "WT Pos",
            NormalisableRange<float> (0.0f, 63.0f, 0.1f), 23.0f));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::aFine, 1 }, "Fine",
            NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f,
            AudioParameterFloatAttributes().withLabel ("ct")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::aLevel, 1 }, "Level", pct(), 100.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterInt>    (ParameterID { pid::aOct, 1 }, "Oct", -3, 3, 0));
    add (std::make_unique<AudioParameterInt>    (ParameterID { pid::aSemi, 1 }, "Semi", -12, 12, 0));

    // ---- OSC B ----
    add (std::make_unique<AudioParameterBool>   (ParameterID { pid::bOn, 1 }, "Osc B", true));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::bWt, 1 }, "WT Pos",
            NormalisableRange<float> (0.0f, 63.0f, 0.1f), 13.6f));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::bFine, 1 }, "Fine",
            NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 4.0f,
            AudioParameterFloatAttributes().withLabel ("ct")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::bLevel, 1 }, "Level", pct(), 35.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::bNoise, 1 }, "Noise", pct(), 0.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterInt>    (ParameterID { pid::bOct, 1 }, "Oct", -3, 3, 0));
    add (std::make_unique<AudioParameterInt>    (ParameterID { pid::bSemi, 1 }, "Semi", -12, 12, 12));

    // ---- HARMONICS ----
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::hCentre, 1 }, "Centre",
            NormalisableRange<float> (0.5f, 4.0f, 0.01f), 1.0f,
            AudioParameterFloatAttributes().withLabel ("x")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::hWidth, 1 },  "Width",  pct(), 30.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::hSpread, 1 }, "Spread", pct(), 100.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::hMix, 1 },    "Mix",    pct(), 0.0f,
            AudioParameterFloatAttributes().withLabel ("%")));

    // ---- CALI wavefolder ----
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::cDrive, 1 },  "Drive",  pct(), 0.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::cShape, 1 },  "Shape",
            NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::cBias, 1 },   "Bias",
            NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::cJitter, 1 }, "Jitter", pct(), 0.0f,
            AudioParameterFloatAttributes().withLabel ("%")));

    // ---- FILTER ----
    add (std::make_unique<AudioParameterChoice> (ParameterID { pid::fType, 1 }, "Filter",
            StringArray { "LP24", "LP12", "BP12", "HP12" }, 0));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::fCutoff, 1 }, "Cutoff",
            NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 5200.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                                                        : juce::String (juce::roundToInt (v)) + " Hz"; })));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::fReso, 1 },  "Reso",  pct(), 20.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::fDrive, 1 }, "Drive", pct(), 45.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::fEnv2, 1 },  "Env 2", bip(), 15.0f,
            AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::fKey, 1 },   "Key",   pct(), 40.0f,
            AudioParameterFloatAttributes().withLabel ("%")));

    // ---- ENVELOPES (3) ----
    auto msTime = [] { return NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.35f); };
    auto msFmt  = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s"
                                                 : juce::String (juce::roundToInt (v)) + " ms"; });
    for (int i = 1; i <= 3; ++i)
    {
        const auto pfx = "Env " + juce::String (i) + " ";
        add (std::make_unique<AudioParameterFloat> (ParameterID { envId (i, "a"), 1 }, pfx + "A", msTime(), i == 1 ? 1400.0f : 5.0f, msFmt));
        add (std::make_unique<AudioParameterFloat> (ParameterID { envId (i, "h"), 1 }, pfx + "H", NormalisableRange<float> (0.0f, 5000.0f, 1.0f, 0.4f), 0.0f, msFmt));
        add (std::make_unique<AudioParameterFloat> (ParameterID { envId (i, "d"), 1 }, pfx + "D", msTime(), 2000.0f, msFmt));
        add (std::make_unique<AudioParameterFloat> (ParameterID { envId (i, "s"), 1 }, pfx + "S", pct(), 85.0f, AudioParameterFloatAttributes().withLabel ("%")));
        add (std::make_unique<AudioParameterFloat> (ParameterID { envId (i, "r"), 1 }, pfx + "R", msTime(), 3500.0f, msFmt));
        add (std::make_unique<AudioParameterFloat> (ParameterID { envId (i, "v"), 1 }, pfx + "Velo", pct(), 60.0f, AudioParameterFloatAttributes().withLabel ("%")));
    }

    // ---- LFOs (3) ----
    for (int i = 1; i <= 3; ++i)
    {
        const auto pfx = "LFO " + juce::String (i) + " ";
        add (std::make_unique<AudioParameterChoice> (ParameterID { lfoId (i, "shape"), 1 }, pfx + "Shape",
                StringArray { "Sine", "Tri", "Saw", "Square", "S&H", "Random" }, 1));
        add (std::make_unique<AudioParameterFloat>  (ParameterID { lfoId (i, "rate"), 1 }, pfx + "Rate",
                NormalisableRange<float> (0.01f, 40.0f, 0.0f, 0.3f), 0.11f,
                AudioParameterFloatAttributes().withLabel ("Hz")));
        add (std::make_unique<AudioParameterBool>   (ParameterID { lfoId (i, "sync"), 1 }, pfx + "Sync", false));
        add (std::make_unique<AudioParameterBool>   (ParameterID { lfoId (i, "retrig"), 1 }, pfx + "Retrig", false));
        add (std::make_unique<AudioParameterFloat>  (ParameterID { lfoId (i, "smooth"), 1 }, pfx + "Smooth", pct(), 0.0f, AudioParameterFloatAttributes().withLabel ("%")));
        add (std::make_unique<AudioParameterFloat>  (ParameterID { lfoId (i, "morph"), 1 }, pfx + "Morph", pct(), 0.0f, AudioParameterFloatAttributes().withLabel ("%")));
        add (std::make_unique<AudioParameterFloat>  (ParameterID { lfoId (i, "rise"), 1 }, pfx + "Rise", NormalisableRange<float> (0.0f, 5000.0f, 1.0f, 0.4f), 0.0f, msFmt));
    }

    // ---- VOICE ----
    add (std::make_unique<AudioParameterChoice> (ParameterID { pid::vMode, 1 }, "Mode",
            StringArray { "Poly", "Mono", "Legato" }, 0));
    add (std::make_unique<AudioParameterInt>    (ParameterID { pid::vVoices, 1 }, "Voices", 1, 16, 16));
    add (std::make_unique<AudioParameterInt>    (ParameterID { pid::vBend, 1 },   "Bend", 0, 24, 2,
            AudioParameterIntAttributes().withLabel ("st")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::vTune, 1 },   "Tune",
            NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("ct")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::vGlide, 1 },  "Glide",
            NormalisableRange<float> (0.0f, 2000.0f, 1.0f, 0.4f), 0.0f, msFmt));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::vDrift, 1 },  "Drift", pct(), 25.0f, AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterBool>   (ParameterID { pid::vMpe, 1 },    "MPE", false));

    // ---- FILTER 2 ----
    add (std::make_unique<AudioParameterChoice> (ParameterID { pid::f2Type, 1 }, "Filter 2",
            StringArray { "Off", "Vowel", "Comb" }, 0));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::f2Reso, 1 },  "Reso",  pct(), 50.0f, AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::f2Mix, 1 },   "Mix",   pct(), 100.0f, AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::f2Orbit, 1 }, "Orbit", pct(), 0.0f, AudioParameterFloatAttributes().withLabel ("%")));
    add (std::make_unique<AudioParameterFloat>  (ParameterID { pid::f2Speed, 1 }, "Speed",
            NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.3f), 0.40f, AudioParameterFloatAttributes().withLabel ("Hz")));
    add (std::make_unique<AudioParameterChoice> (ParameterID { pid::f2Route, 1 }, "Route",
            StringArray { "Serial", "Parallel" }, 0));

    return L;
}
