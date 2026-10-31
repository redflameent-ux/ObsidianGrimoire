#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "../redflame/RedFlameMod.h"

// FOUNDATION - saturation + loudness (effect).
class FoundationProcessor : public juce::AudioProcessor
{
public:
    FoundationProcessor();
    ~FoundationProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FOUNDATION"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Bus Ignition"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    rf::ModMatrix modMatrix;
    std::atomic<float> inLufs { -60.0f }, outLufs { -60.0f }, reduceDb { 0.0f }, limitDb { 0.0f };

private:
    double sr = 44100.0;
    float compEnv = 0.0f, limEnv = 0.0f;
    float inRms = 0.0f, outRms = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FoundationProcessor)
};
