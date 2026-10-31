#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// AETHER - sample-stack synth. Global params + a "selected layer" proxy set (lsel_*)
// that the editor syncs to/from the current layer in LayerData.h.
namespace aid
{
    constexpr auto master = "master";

    // selected-layer proxy
    constexpr auto lLevel = "l_level", lPan = "l_pan", lCoarse = "l_coarse", lFine = "l_fine";
    constexpr auto lRoot = "l_root", lStart = "l_start", lLoopxf = "l_loopxf", lDrift = "l_drift";
    constexpr auto lGsize = "l_gsize", lGdensity = "l_gdensity", lGspray = "l_gspray", lGrandom = "l_grandom";

    // FILTER A / B
    constexpr auto fAon = "fa_on", fAtype = "fa_type", fAcut = "fa_cut", fAres = "fa_res", fAdrive = "fa_drive", fAenv2 = "fa_env2", fAkey = "fa_key", fAmix = "fa_mix";
    constexpr auto fBon = "fb_on", fBtype = "fb_type", fBcut = "fb_cut", fBres = "fb_res", fBdrive = "fb_drive", fBenv2 = "fb_env2", fBkey = "fb_key", fBmix = "fb_mix";
    constexpr auto routing = "routing";

    // PATINA
    constexpr auto pMachine = "p_machine", pDrift = "p_drift", pVariance = "p_variance", pAge = "p_age";
    constexpr auto pVca = "p_vca", pTape = "p_tape", pWobble = "p_wobble", pNoise = "p_noise";

    // ENV (amp)
    constexpr auto eA = "e_a", eH = "e_h", eD = "e_d", eS = "e_s", eR = "e_r", eCurve = "e_curve", eVelo = "e_velo";

    // MACROS
    constexpr auto m1 = "m1", m2 = "m2", m3 = "m3", m4 = "m4", m5 = "m5", m6 = "m6", m7 = "m7", m8 = "m8";

    // SCALE
    constexpr auto quantize = "quantize", key = "key", scale = "scale";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout aetherLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct = [] { return NormalisableRange<float> (0.0f, 100.0f, 0.1f); };
    auto bip = [] { return NormalisableRange<float> (-100.0f, 100.0f, 0.1f); };
    auto hzF = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (roundToInt (v)) + " Hz"; });
    auto& A = L;
    auto f = [&] (const char* id, const char* nm, NormalisableRange<float> r, float d, const char* lbl = "%")
        { A.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d, AudioParameterFloatAttributes().withLabel (lbl))); };
    auto ff = [&] (const char* id, const char* nm, NormalisableRange<float> r, float d, AudioParameterFloatAttributes a)
        { A.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d, a)); };
    auto b = [&] (const char* id, const char* nm, bool d) { A.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, d)); };
    auto c = [&] (const char* id, const char* nm, StringArray o, int d) { A.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, o, d)); };

    f (aid::master, "Master", { -60.0f, 12.0f, 0.1f }, 0.0f, "dB");

    // selected layer
    f  (aid::lLevel, "Level", pct(), 80.0f);
    f  (aid::lPan,   "Pan",   bip(), 0.0f);
    f  (aid::lCoarse,"Coarse",{ -24.0f, 24.0f, 1.0f }, 0.0f, "st");
    f  (aid::lFine,  "Fine",  { -100.0f, 100.0f, 1.0f }, 0.0f, "ct");
    f  (aid::lRoot,  "Root",  { 0.0f, 127.0f, 1.0f }, 60.0f, "");
    f  (aid::lStart, "Start", pct(), 0.0f);
    f  (aid::lLoopxf,"Loop X-Fade", pct(), 10.0f);
    f  (aid::lDrift, "Drift", pct(), 15.0f);
    ff (aid::lGsize, "Size", { 1.0f, 500.0f, 1.0f }, 80.0f, AudioParameterFloatAttributes().withLabel ("ms"));
    ff (aid::lGdensity, "Density", { 0.1f, 20.0f, 0.1f }, 3.0f, AudioParameterFloatAttributes().withLabel ("gr"));
    f  (aid::lGspray, "Spray", pct(), 10.0f);
    f  (aid::lGrandom,"Random", pct(), 0.0f);

    auto filter = [&] (const char* on, const char* type, const char* cut, const char* res, const char* drv,
                       const char* env2, const char* key, const char* mix, int typeDef)
    {
        b (on, "On", true);
        c (type, "Type", { "Low Pass", "High Pass", "Band Pass" }, typeDef);
        ff (cut, "Cutoff", { 20.0f, 20000.0f, 0.0f, 0.3f }, 1800.0f, hzF);
        f (res, "Res", pct(), 55.0f); f (drv, "Drive", pct(), 10.0f);
        f (env2, "Env 2", bip(), 30.0f); f (key, "Key", pct(), 0.0f); f (mix, "Mix", pct(), 100.0f);
    };
    filter (aid::fAon, aid::fAtype, aid::fAcut, aid::fAres, aid::fAdrive, aid::fAenv2, aid::fAkey, aid::fAmix, 0);
    filter (aid::fBon, aid::fBtype, aid::fBcut, aid::fBres, aid::fBdrive, aid::fBenv2, aid::fBkey, aid::fBmix, 1);
    c (aid::routing, "Route", { "A into B (Serial)", "A + B (Parallel)" }, 0);

    // PATINA
    c (aid::pMachine, "Machine", { "SP-12", "MPC", "VHS", "Cassette", "Dust" }, 0);
    f (aid::pDrift, "Drift", pct(), 35.0f); f (aid::pVariance, "Variance", pct(), 25.0f);
    f (aid::pAge, "Age", pct(), 0.0f); f (aid::pVca, "VCA", pct(), 15.0f);
    f (aid::pTape, "Tape", pct(), 25.0f); f (aid::pWobble, "Wobble", pct(), 0.0f); f (aid::pNoise, "Noise", pct(), 0.0f);

    // ENV
    auto ms = NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.35f);
    auto msF = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (roundToInt (v)) + " ms"; });
    ff (aid::eA, "Attack", ms, 0.0f, msF); ff (aid::eH, "Hold", NormalisableRange<float> (0.0f, 5000.0f, 1.0f, 0.4f), 0.0f, msF);
    ff (aid::eD, "Decay", ms, 600.0f, msF); f (aid::eS, "Sustain", pct(), 100.0f); ff (aid::eR, "Release", ms, 250.0f, msF);
    f (aid::eCurve, "Curve", bip(), 55.0f); f (aid::eVelo, "Velo", pct(), 40.0f);

    // MACROS
    for (auto* id : { aid::m1, aid::m2, aid::m3, aid::m4, aid::m5, aid::m6, aid::m7, aid::m8 })
        f (id, "Macro", pct(), 0.0f);

    // SCALE
    b (aid::quantize, "Quantize", false);
    c (aid::key, "Key", { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 0);
    c (aid::scale, "Scale", { "Chromatic", "Major", "Minor", "Pentatonic", "Dorian" }, 0);

    return L;
}
