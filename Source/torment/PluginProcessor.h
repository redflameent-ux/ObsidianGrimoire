#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "../redflame/RedFlameMod.h"

// TORMENT - hi-fi lo-fi processor (effect). Per-sample chain: tape drive + hiss ->
// bit/rate crush -> vinyl noise + tone -> dry/wet mix -> output.
class TormentProcessor : public juce::AudioProcessor
{
public:
    TormentProcessor();
    ~TormentProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TORMENT"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    rf::ModMatrix modMatrix;
    std::atomic<float> scopeLevel { 0.0f };   // for the CRT display

private:
    double sr = 44100.0;
    float sh[2] = { 0.0f, 0.0f };
    float shCount[2] = { 0.0f, 0.0f };
    float lp[2] = { 0.0f, 0.0f };
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TormentProcessor)
};
