#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// FOUNDATION - saturation + loudness (effect, modelled on EMBER). Signal flow:
// input -> saturate -> compress -> loudness -> limit -> output.
namespace fnd
{
    constexpr auto master = "master";

    // INPUT
    constexpr auto input = "input", quality = "quality", live = "live";

    // SATURATE
    constexpr auto bass = "bass", thick = "thick", depth = "depth", sub = "sub", focus = "focus";
    constexpr auto spread = "spread", phantom = "phantom", root = "root", color = "color";

    // COMPRESS
    constexpr auto compMode = "comp_mode", comp = "comp", body = "body", clip = "clip", soft = "soft", crunch = "crunch";

    // LOUD
    constexpr auto loud = "loud", punch = "punch", clarity = "clarity", shape = "shape", width = "width", target = "target";

    // OUTPUT
    constexpr auto mix = "mix", output = "output", bypass = "bypass";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout foundationLayout()
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

    f (fnd::master, "Master", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");

    // INPUT
    f (fnd::input, "Input", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    c (fnd::quality, "Quality", { "Off", "2X", "4X" }, 1);
    b (fnd::live, "Live", false);

    // SATURATE
    f (fnd::bass, "Bass", bip(), 0.0f); f (fnd::thick, "Thick", pct(), 30.0f); f (fnd::depth, "Depth", pct(), 0.0f);
    f (fnd::sub, "Sub", pct(), 0.0f); ff (fnd::focus, "Focus", { 20.0f, 20000.0f, 0.0f, 0.3f }, 1000.0f, hzF); f (fnd::spread, "Spread", pct(), 0.0f);
    f (fnd::phantom, "Phantom", pct(), 0.0f); ff (fnd::root, "Root", { 20.0f, 500.0f, 0.0f, 0.3f }, 69.0f, hzF); f (fnd::color, "Color", bip(), 0.0f);

    // COMPRESS
    c (fnd::compMode, "Mode", { "Comp", "OTT" }, 0);
    f (fnd::comp, "Comp", pct(), 25.0f); f (fnd::body, "Body", pct(), 0.0f);
    b (fnd::clip, "Clip", false); f (fnd::soft, "Soft", pct(), 0.0f); f (fnd::crunch, "Crunch", pct(), 0.0f);

    // LOUD
    f (fnd::loud, "Loud", pct(), 0.0f); f (fnd::punch, "Punch", pct(), 0.0f); f (fnd::clarity, "Clarity", pct(), 0.0f);
    f (fnd::shape, "Shape", pct(), 0.0f); f (fnd::width, "Width", pct(), 100.0f); f (fnd::target, "Target", { -24.0f, -6.0f, 0.1f }, -14.0f, "LUFS");

    // OUTPUT
    f (fnd::mix, "Mix", pct(), 100.0f); f (fnd::output, "Output", { -24.0f, 12.0f, 0.1f }, 0.0f, "dB"); b (fnd::bypass, "Bypass", false);

    return L;
}
