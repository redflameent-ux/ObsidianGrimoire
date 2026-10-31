#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Pattern.h"
#include "../redflame/RedFlameMod.h"

// one synthesized drum voice; retriggered per lane
struct DrumGen
{
    int type = 0; bool active = false; int t = 0; float vel = 0.0f, pan = 0.0f;
    double phase = 0.0; float hp = 0.0f, prevN = 0.0f; juce::Random rng;

    void trigger (int typeIndex, float v, float p) { type = typeIndex; vel = v; pan = p; t = 0; phase = 0.0; active = true; }

    float next (double sr)
    {
        if (! active) return 0.0f;
        const double tt = t / sr;
        const float n = rng.nextFloat() * 2.0f - 1.0f;
        float out = 0.0f;
        auto sine = [&] (double f) { phase += f / sr; return (float) std::sin (phase * juce::MathConstants<double>::twoPi); };
        const float hpN = n - prevN; prevN = n;   // crude high-pass for noise
        switch (type)
        {
            case 0: { out = sine (45.0 + 75.0 * std::exp (-tt / 0.03)) * std::exp (-tt / 0.18f); if (tt > 0.6) active = false; } break; // kick
            case 1: { out = (sine (180.0) * 0.5f * std::exp (-tt / 0.10f)) + hpN * std::exp (-tt / 0.14f); if (tt > 0.3) active = false; } break; // snare
            case 2: { out = hpN * std::exp (-tt / 0.025f) * 0.7f; if (tt > 0.09) active = false; } break; // c-hat
            case 3: { out = hpN * std::exp (-tt / 0.18f) * 0.6f;  if (tt > 0.35) active = false; } break; // o-hat
            case 4: { out = hpN * std::exp (-tt / 0.12f) * (0.6f + 0.4f * std::sin (tt * 900.0)); if (tt > 0.25) active = false; } break; // clap
            case 5: { out = sine (180.0 * std::exp (-tt / 0.10)) * std::exp (-tt / 0.22f); if (tt > 0.4) active = false; } break; // tom
            case 6: { out = (sine (1700.0) * 0.6f + hpN * 0.5f) * std::exp (-tt / 0.02f); if (tt > 0.06) active = false; } break; // rim
            default:{ out = (hpN * 0.6f + sine (5200.0) * 0.1f) * std::exp (-tt / 0.40f); if (tt > 0.6) active = false; } break; // ride
        }
        ++t;
        return out * vel * 0.7f;
    }
};

class IncantProcessor : public juce::AudioProcessor
{
public:
    IncantProcessor();
    ~IncantProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "INCANT"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // called from the UI
    void generate (int genre);

    juce::AudioProcessorValueTreeState apvts;
    rf::ModMatrix modMatrix;
    incant::Pattern pattern;
    std::atomic<int>  uiStep { -1 };
    std::atomic<bool> uiPlaying { false };

private:
    void triggerStep (int s);

    double sr = 44100.0;
    double stepPhase = 0.0, stepDur = 11025.0;
    int step = 0;
    bool prevRun = false;
    DrumGen drums[incant::kLanes];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IncantProcessor)
};
