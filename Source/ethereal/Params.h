#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// ETHEREAL - galactic space reverb (modelled on the DEEPFIELD reference).
// An effect, not an instrument. The DSP wires the core reverb path (drive, pre-delay,
// lo/hi cut, size/decay/damping/width/freeze, dry-wet mix, output); the rest is UI.
namespace eid
{
    constexpr auto master = "master";
    constexpr auto limit  = "limit";

    // STACK
    constexpr auto slotA   = "slot_a";
    constexpr auto levelA  = "level_a";
    constexpr auto slotB   = "slot_b";
    constexpr auto levelB  = "level_b";
    constexpr auto routing = "routing";

    // GALACTIC SHIFT
    constexpr auto morph     = "morph";
    constexpr auto orbit     = "orbit";
    constexpr auto reach     = "reach";
    constexpr auto shiftMode = "shift_mode";

    // SPIRAL
    constexpr auto path     = "path";
    constexpr auto spin     = "spin";
    constexpr auto spiral   = "spiral";
    constexpr auto height   = "height";
    constexpr auto position = "position";

    // SPACE
    constexpr auto size    = "size";
    constexpr auto decay   = "decay";
    constexpr auto mass    = "mass";
    constexpr auto damp    = "damp";
    constexpr auto diffuse = "diffuse";
    constexpr auto modd    = "mod";
    constexpr auto drift   = "drift";

    // INPUT
    constexpr auto drive    = "drive";
    constexpr auto preDelay = "predelay";
    constexpr auto loCut    = "locut";
    constexpr auto hiCut    = "hicut";

    // OUTPUT
    constexpr auto width    = "width";
    constexpr auto makeup   = "makeup";
    constexpr auto comp     = "comp";
    constexpr auto eclipse  = "eclipse";
    constexpr auto autoGain = "autogain";
    constexpr auto freeze   = "freeze";
    constexpr auto mix      = "mix";

    // PARTIALS
    constexpr auto partType = "part_type";
    constexpr auto pAmount  = "p_amount";
    constexpr auto pPitch   = "p_pitch";
    constexpr auto pCount   = "p_count";
    constexpr auto pRing    = "p_ring";
    constexpr auto pShimmer = "p_shimmer";
    constexpr auto pTilt    = "p_tilt";
    constexpr auto pSpread  = "p_spread";

    // RUMBLE
    constexpr auto rumble   = "rumble";
    constexpr auto sub      = "sub";
    constexpr auto rDepth   = "r_depth";
    constexpr auto roll     = "roll";
    constexpr auto dark     = "dark";

    // IN/OUT
    constexpr auto inGain   = "in_gain";
    constexpr auto stereoMode = "stereo_mode";
    constexpr auto oversample = "oversample";
    constexpr auto quality  = "quality";
    constexpr auto hq       = "hq";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout etherealLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct = [] { return NormalisableRange<float> (0.0f, 100.0f, 0.1f); };
    auto hz  = [] (float lo, float hi, float d) { return NormalisableRange<float> (lo, hi, 0.0f, 0.3f); };
    auto hzF = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (roundToInt (v)) + " Hz"; });
    auto pctA = AudioParameterFloatAttributes().withLabel ("%");
    auto dbA  = AudioParameterFloatAttributes().withLabel ("dB");
    auto& A = L;
    auto f = [&] (const char* id, const char* nm, NormalisableRange<float> r, float d, AudioParameterFloatAttributes a = {})
        { A.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d, a)); };
    auto b = [&] (const char* id, const char* nm, bool d) { A.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, d)); };
    auto c = [&] (const char* id, const char* nm, StringArray o, int d) { A.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, o, d)); };
    auto in = [&] (const char* id, const char* nm, int lo, int hi, int d) { A.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, nm, lo, hi, d)); };

    const StringArray slots { "Observatory", "Singularity", "Nebula", "Void", "Cathedral", "Bloom" };

    f (eid::master, "Master", { -60.0f, 12.0f, 0.1f }, 0.0f, dbA);
    b (eid::limit, "Limit", true);

    // STACK
    c (eid::slotA, "Slot A", slots, 0);
    f (eid::levelA, "Level A", { -60.0f, 6.0f, 0.1f }, -3.0f, dbA);
    c (eid::slotB, "Slot B", slots, 1);
    f (eid::levelB, "Level B", { -60.0f, 6.0f, 0.1f }, -3.0f, dbA);
    c (eid::routing, "Routing", { "Parallel", "Series" }, 0);

    // GALACTIC SHIFT
    f (eid::morph, "Morph", pct(), 50.0f, pctA);
    f (eid::orbit, "Orbit", hz (0.01f, 10.0f, 0.4f), 0.40f, AudioParameterFloatAttributes().withLabel ("Hz"));
    f (eid::reach, "Reach", pct(), 70.0f, pctA);
    c (eid::shiftMode, "Shift", { "Smooth", "Triangle", "Drift" }, 0);

    // SPIRAL
    c (eid::path, "Path", { "Spiral", "Orbit", "Pendulum", "Random" }, 0);
    f (eid::spin,   "Spin",   hz (0.01f, 10.0f, 0.6f), 0.60f, AudioParameterFloatAttributes().withLabel ("Hz"));
    f (eid::spiral, "Spiral", pct(), 85.0f, pctA);
    f (eid::height, "Height", pct(), 60.0f, pctA);
    f (eid::position, "Position", pct(), 50.0f, pctA);

    // SPACE
    f (eid::size,  "Size",  pct(), 63.0f, pctA);
    f (eid::decay, "Decay", { 0.1f, 20.0f, 0.0f, 0.4f }, 2.50f, AudioParameterFloatAttributes().withLabel ("s"));
    f (eid::mass,  "Mass",  pct(), 0.0f, pctA);
    f (eid::damp,  "Damp",  hz (200.0f, 18000.0f, 0.3f), 7000.0f, hzF);
    f (eid::diffuse, "Diffuse", pct(), 70.0f, pctA);
    f (eid::modd,  "Mod",   pct(), 30.0f, pctA);
    f (eid::drift, "Drift", pct(), 25.0f, pctA);

    // INPUT
    f (eid::drive,    "Drive", pct(), 0.0f, pctA);
    f (eid::preDelay, "Pre-Delay", { 0.0f, 250.0f, 1.0f }, 20.0f, AudioParameterFloatAttributes().withLabel ("ms"));
    f (eid::loCut,    "Lo Cut", hz (20.0f, 2000.0f, 0.3f), 60.0f, hzF);
    f (eid::hiCut,    "Hi Cut", hz (1000.0f, 20000.0f, 0.3f), 14000.0f, hzF);

    // OUTPUT
    f (eid::width,   "Width",  pct(), 100.0f, pctA);
    f (eid::makeup,  "Makeup", { -12.0f, 12.0f, 0.1f }, 0.0f, dbA);
    f (eid::comp,    "Comp",   pct(), 0.0f, pctA);
    f (eid::eclipse, "Eclipse",pct(), 0.0f, pctA);
    b (eid::autoGain, "Auto Gain", false);
    b (eid::freeze,   "Freeze", false);
    f (eid::mix,     "Mix", pct(), 35.0f, pctA);

    // PARTIALS
    c (eid::partType, "Type", { "Harmonic", "Inharmonic", "Octaves", "Fifths" }, 0);
    f (eid::pAmount,  "Amount", pct(), 0.0f, pctA);
    f (eid::pPitch,   "Pitch", hz (20.0f, 2000.0f, 0.3f), 110.0f, hzF);
    in (eid::pCount,  "Count", 1, 48, 12);
    f (eid::pRing,    "Ring", { 0.1f, 10.0f, 0.0f, 0.4f }, 2.0f, AudioParameterFloatAttributes().withLabel ("s"));
    f (eid::pShimmer, "Shimmer", pct(), 35.0f, pctA);
    f (eid::pTilt,    "Tilt", pct(), 0.0f, pctA);
    f (eid::pSpread,  "Spread", pct(), 70.0f, pctA);

    // RUMBLE
    f (eid::rumble, "Rumble", pct(), 0.0f, pctA);
    f (eid::sub,    "Sub", pct(), 0.0f, pctA);
    f (eid::rDepth, "Depth", hz (20.0f, 200.0f, 0.3f), 50.0f, hzF);
    f (eid::roll,   "Roll", pct(), 40.0f, pctA);
    f (eid::dark,   "Dark", pct(), 0.0f, pctA);

    // IN/OUT
    f (eid::inGain, "Input", { -24.0f, 24.0f, 0.1f }, 0.0f, dbA);
    c (eid::stereoMode, "Stereo", { "Stereo", "Mid", "Side" }, 0);
    c (eid::oversample, "Oversample", { "Off", "2x", "4x" }, 1);
    c (eid::quality, "Quality", { "Fast", "Linear" }, 0);
    c (eid::hq, "HQ", { "HQ", "Eco" }, 0);

    return L;
}
