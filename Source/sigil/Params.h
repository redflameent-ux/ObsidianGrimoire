#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// SIGIL - modal resonator synthesizer (modelled on the RESONA reference).
// Parameter ids for the SYNTH page. Not every one drives DSP yet; the resonator
// voice (Karplus-Strong) reads strike/structure/brightness/damping/position + the
// exciter envelope + pitch. The rest are UI-wired for now.
namespace sid
{
    constexpr auto master   = "master";

    // EXCITER
    constexpr auto strike   = "strike";
    constexpr auto bowblow  = "bowblow";
    constexpr auto color    = "color";
    constexpr auto osc      = "osc";
    constexpr auto shape    = "shape";
    constexpr auto env      = "exc_env";   // Amp / Exciter (which env the ADSR edits)
    constexpr auto atk      = "atk";
    constexpr auto dec      = "dec";
    constexpr auto sus      = "sus";
    constexpr auto rel      = "rel";

    // RESONATOR
    constexpr auto engine   = "engine";    // Modal / Sympathetic / Inharmonic / FM / Chords
    constexpr auto structure = "structure";
    constexpr auto bright    = "bright";
    constexpr auto damping   = "damping";
    constexpr auto position  = "position";
    constexpr auto keyUpDamp = "keyupdamp";
    constexpr auto chord    = "chord";
    constexpr auto key      = "key";
    constexpr auto scale    = "scale";

    // SPACE
    constexpr auto chorusOn   = "chorus_on";
    constexpr auto chorusType = "chorus_type";
    constexpr auto chorusAmt  = "chorus_amt";
    constexpr auto chorusDepth= "chorus_depth";
    constexpr auto chorusRate = "chorus_rate";
    constexpr auto chorusWidth= "chorus_width";
    constexpr auto reverbOn   = "reverb_on";
    constexpr auto reverbMix  = "reverb_mix";
    constexpr auto reverbTime = "reverb_time";
    constexpr auto reverbTone = "reverb_tone";
    constexpr auto reverbType = "reverb_type";
    constexpr auto reverbSync = "reverb_sync";

    // MOTION
    constexpr auto motRate   = "mot_rate";
    constexpr auto motShape  = "mot_shape";
    constexpr auto motStruct = "mot_struct";
    constexpr auto motBright = "mot_bright";
    constexpr auto motPos    = "mot_pos";
    constexpr auto motVib    = "mot_vib";
    constexpr auto motWt     = "mot_wt";

    // PLAY
    constexpr auto voices    = "voices";
    constexpr auto mode      = "mode";
    constexpr auto glide     = "glide";
    constexpr auto octave    = "octave";
    constexpr auto tune      = "tune";

    // TOUCH
    constexpr auto velBrt    = "vel_brt";
    constexpr auto velStk    = "vel_stk";
    constexpr auto mwLfo     = "mw_lfo";
    constexpr auto mwBrt     = "mw_brt";
    constexpr auto atExc     = "at_exc";
    constexpr auto bend      = "bend";

    // MACROS
    constexpr auto m1 = "m1", m2 = "m2", m3 = "m3", m4 = "m4";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout sigilLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct = [] { return NormalisableRange<float> (0.0f, 100.0f, 0.1f); };
    auto ms  = [] { return NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.35f); };
    auto msF = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (roundToInt (v)) + " ms"; });
    auto pctA = AudioParameterFloatAttributes().withLabel ("%");
    auto& add = L;
    auto f = [&] (const char* id, const char* nm, NormalisableRange<float> r, float d, AudioParameterFloatAttributes a = {})
        { add.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d, a)); };
    auto b = [&] (const char* id, const char* nm, bool d) { add.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, d)); };
    auto c = [&] (const char* id, const char* nm, StringArray o, int d) { add.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, o, d)); };
    auto in = [&] (const char* id, const char* nm, int lo, int hi, int d, const char* lbl = "")
        { add.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, nm, lo, hi, d, AudioParameterIntAttributes().withLabel (lbl))); };

    f (sid::master, "Master", { -60.0f, 12.0f, 0.1f }, 0.0f, AudioParameterFloatAttributes().withLabel ("dB"));

    // EXCITER
    f (sid::strike,  "Strike",   pct(), 100.0f, pctA);
    f (sid::bowblow, "Bow/Blow", pct(), 0.0f,   pctA);
    f (sid::color,   "Color",    pct(), 50.0f,  pctA);
    f (sid::osc,     "Osc",      pct(), 0.0f,   pctA);
    f (sid::shape,   "Shape",    pct(), 50.0f,  pctA);
    c (sid::env,     "Env", { "Amp", "Exciter" }, 0);
    f (sid::atk, "Attack",  { 0.0f, 2000.0f, 1.0f, 0.4f }, 1.0f, msF);
    f (sid::dec, "Decay",   ms(), 1000.0f, msF);
    f (sid::sus, "Sustain", pct(), 100.0f, pctA);
    f (sid::rel, "Release", ms(), 4600.0f, msF);

    // RESONATOR
    c (sid::engine, "Engine", { "Modal", "Sympathetic", "Inharmonic", "FM", "Chords" }, 0);
    f (sid::structure, "Structure",  pct(), 25.0f, pctA);
    f (sid::bright,    "Brightness", pct(), 50.0f, pctA);
    f (sid::damping,   "Damping",    pct(), 75.0f, pctA);
    f (sid::position,  "Position",   pct(), 48.0f, pctA);
    f (sid::keyUpDamp, "Key-Up Damp", pct(), 100.0f, pctA);
    c (sid::chord, "Chord", { "Off", "Octave", "Fifth", "Sus4", "Maj", "Min", "Maj7", "Min7" }, 0);
    c (sid::key,   "Key",   { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 0);
    c (sid::scale, "Scale", { "Chromatic", "Major", "Minor", "Pentatonic", "Whole Tone" }, 0);

    // SPACE
    b (sid::chorusOn,   "Chorus", true);
    c (sid::chorusType, "Type", { "Ensemble", "Chorus", "Flanger" }, 0);
    f (sid::chorusAmt,  "Amount", pct(), 25.0f, pctA);
    f (sid::chorusDepth,"Depth",  pct(), 50.0f, pctA);
    f (sid::chorusRate, "Rate", { 0.01f, 10.0f, 0.0f, 0.3f }, 0.8f, AudioParameterFloatAttributes().withLabel ("Hz"));
    f (sid::chorusWidth,"Width",  pct(), 50.0f, pctA);
    b (sid::reverbOn,   "Reverb", true);
    f (sid::reverbMix,  "Mix",  pct(), 25.0f, pctA);
    f (sid::reverbTime, "Time", pct(), 50.0f, pctA);
    f (sid::reverbTone, "Tone", pct(), 50.0f, pctA);
    c (sid::reverbType, "Rev Type", { "Plate", "Hall", "Room", "Shimmer" }, 0);
    b (sid::reverbSync, "Sync", false);

    // MOTION
    f (sid::motRate, "Rate", { 0.01f, 20.0f, 0.0f, 0.3f }, 0.1f, AudioParameterFloatAttributes().withLabel ("Hz"));
    c (sid::motShape, "Shape", { "Sine", "Tri", "Saw", "Square", "Random" }, 0);
    f (sid::motStruct, "Struct",  pct(), 0.0f, pctA);
    f (sid::motBright, "Bright",  pct(), 0.0f, pctA);
    f (sid::motPos,    "Position",pct(), 0.0f, pctA);
    f (sid::motVib,    "Vibrato", pct(), 0.0f, pctA);
    f (sid::motWt,     "WT",      pct(), 0.0f, pctA);

    // PLAY
    in (sid::voices, "Voices", 1, 8, 6);
    c  (sid::mode,   "Mode", { "Poly", "Mono", "Legato" }, 0);
    f  (sid::glide,  "Glide", ms(), 0.0f, msF);
    in (sid::octave, "Octave", -3, 3, 0);
    f  (sid::tune,   "Tune", { -100.0f, 100.0f, 0.1f }, 0.0f, AudioParameterFloatAttributes().withLabel ("ct"));

    // TOUCH
    f (sid::velBrt, "Vel-Brt", { -100.0f, 100.0f, 0.1f }, 50.0f, pctA);
    f (sid::velStk, "Vel-Stk", pct(), 75.0f, pctA);
    f (sid::mwLfo,  "MW-LFO",  pct(), 50.0f, pctA);
    f (sid::mwBrt,  "MW-Brt",  pct(), 0.0f,  pctA);
    f (sid::atExc,  "AT-Exc",  pct(), 50.0f, pctA);
    in (sid::bend,  "Bend", 0, 24, 2, "st");

    // MACROS
    f (sid::m1, "Macro 1", pct(), 0.0f, pctA); f (sid::m2, "Macro 2", pct(), 0.0f, pctA);
    f (sid::m3, "Macro 3", pct(), 0.0f, pctA); f (sid::m4, "Macro 4", pct(), 0.0f, pctA);

    return L;
}
