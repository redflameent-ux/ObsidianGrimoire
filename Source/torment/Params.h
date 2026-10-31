#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// TORMENT - hi-fi lo-fi processor (effect, modelled on SILTH). DSP wires the core
// lo-fi path (drive, bit/rate crush, noise/hiss, tone, dry-wet); grain/capture are UI.
namespace tid
{
    constexpr auto master = "master";

    // AGE (global blend)
    constexpr auto age = "age", mix = "mix", punch = "punch";
    constexpr auto tapeAmt = "tape_amt", decAmt = "dec_amt", vinAmt = "vin_amt";

    // TAPE
    constexpr auto tDrive = "t_drive", tBump = "t_bump", tFlutter = "t_flutter", tHiss = "t_hiss";

    // DECIMATE
    constexpr auto dMachine = "d_machine", dUlaw = "d_ulaw", dRate = "d_rate", dBits = "d_bits", dSmooth = "d_smooth", dJitter = "d_jitter";

    // VINYL
    constexpr auto vSpeed = "v_speed", vDust = "v_dust", vPops = "v_pops", vRumble = "v_rumble", vWow = "v_wow", vTone = "v_tone", vMono = "v_mono";

    // GRAIN
    constexpr auto gFreeze = "g_freeze", gShape = "g_shape", gScale = "g_scale", gSync = "g_sync";
    constexpr auto gGrain = "g_grain", gSize = "g_size", gDensity = "g_density", gSpray = "g_spray", gDetune = "g_detune";
    constexpr auto gWidth = "g_width", gReverse = "g_reverse", gPitch = "g_pitch", gWarp = "g_warp", gPitchMode = "g_pitchmode";
    constexpr auto gPositn = "g_positn", gFdbk = "g_fdbk", gChance = "g_chance", gOctave = "g_octave", gJitter = "g_jitter", gColor = "g_color", gReso = "g_reso";

    // CAPTURE
    constexpr auto capLen = "cap_len", capMode = "cap_mode", bpm = "bpm";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout tormentLayout()
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

    f (tid::master, "Master", { -60.0f, 12.0f, 0.1f }, 0.0f, "dB");

    // AGE
    f (tid::age, "Age", pct(), 50.0f); f (tid::mix, "Mix", pct(), 100.0f); f (tid::punch, "Punch", pct(), 75.0f);
    f (tid::tapeAmt, "Tape", pct(), 100.0f); f (tid::decAmt, "Decimate", pct(), 100.0f); f (tid::vinAmt, "Vinyl", pct(), 100.0f);

    // TAPE
    f (tid::tDrive, "Drive", pct(), 0.0f); f (tid::tBump, "Bump", pct(), 0.0f);
    f (tid::tFlutter, "Flutter", pct(), 0.0f); f (tid::tHiss, "Hiss", pct(), 0.0f);

    // DECIMATE
    c (tid::dMachine, "Machine", { "Choose", "SP-1200", "MPC-60", "S950", "Casio", "PCM" }, 0);
    b (tid::dUlaw, "U-Law", false);
    f (tid::dRate, "Rate", pct(), 0.0f); f (tid::dBits, "Bits", pct(), 0.0f);
    f (tid::dSmooth, "Smooth", pct(), 0.0f); f (tid::dJitter, "Jitter", pct(), 0.0f);

    // VINYL
    c (tid::vSpeed, "Speed", { "33", "45", "78" }, 0);
    f (tid::vDust, "Dust", pct(), 0.0f); f (tid::vPops, "Pops", pct(), 0.0f); f (tid::vRumble, "Rumble", pct(), 0.0f);
    f (tid::vWow, "Wow", pct(), 0.0f); f (tid::vTone, "Tone", pct(), 50.0f); f (tid::vMono, "Mono", pct(), 0.0f);

    // GRAIN
    b (tid::gFreeze, "Freeze", false);
    c (tid::gShape, "Shape", { "Smooth", "Sharp", "Gauss", "Tukey" }, 0);
    c (tid::gScale, "Scale", { "Free", "Major", "Minor", "Penta" }, 0);
    c (tid::gSync, "Sync", { "1/16", "1/8", "1/4", "1/2", "1 Bar" }, 0);
    f (tid::gGrain, "Grain", pct(), 0.0f); f (tid::gSize, "Size", { 1.0f, 500.0f, 1.0f }, 80.0f, "ms");
    f (tid::gDensity, "Density", { 0.1f, 20.0f, 0.1f }, 3.0f, "x"); f (tid::gSpray, "Spray", pct(), 0.0f);
    f (tid::gDetune, "Detune", bip(), 0.0f, "st"); f (tid::gWidth, "Width", pct(), 0.0f);
    f (tid::gReverse, "Reverse", pct(), 0.0f); f (tid::gPitch, "Pitch", { -24.0f, 24.0f, 1.0f }, 0.0f, "st");
    f (tid::gWarp, "Warp", { 0.25f, 4.0f, 0.01f }, 1.0f, "x"); c (tid::gPitchMode, "Pitch", { "Hold", "Bend" }, 0);
    f (tid::gPositn, "Positn", { 0.0f, 1000.0f, 1.0f }, 0.0f, "ms"); f (tid::gFdbk, "Fdbk", pct(), 0.0f);
    f (tid::gChance, "Chance", pct(), 100.0f); f (tid::gOctave, "Octave", { -3.0f, 3.0f, 1.0f }, 0.0f, "");
    f (tid::gJitter, "Jitter", pct(), 0.0f); f (tid::gColor, "Color", bip(), 0.0f); f (tid::gReso, "Reso", pct(), 0.0f);

    // CAPTURE
    c (tid::capLen, "Length", { "1/8", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars" }, 3);
    c (tid::capMode, "Mode", { "Song", "Set" }, 0);
    f (tid::bpm, "BPM", { 40.0f, 300.0f, 0.1f }, 120.0f, "");

    return L;
}
