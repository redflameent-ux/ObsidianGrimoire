#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "../redflame/RedFlameMod.h"

// ETHEREAL - galactic space reverb (effect). Input -> drive -> pre-delay -> lo/hi cut
// -> reverb -> dry/wet mix -> output. Validates the shared kit on an effect (no voice,
// no keyboard).
class EtherealProcessor : public juce::AudioProcessor
{
public:
    EtherealProcessor();
    ~EtherealProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ETHEREAL"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    rf::ModMatrix modMatrix;

private:
    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;
    Delay preDelay { 44100 };
    juce::dsp::StateVariableTPTFilter<float> loCut, hiCut;
    juce::Reverb reverb;
    juce::AudioBuffer<float> dryBuf;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EtherealProcessor)
};
