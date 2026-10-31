#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "LayerData.h"
#include "../redflame/RedFlameMod.h"

// AETHER - sample-stack synth. Each voice sums the ON layers' oscillators (a synth
// stand-in for sampled content) through the amp envelope, then a global ladder filter.
class AetherProcessor : public juce::AudioProcessor
{
public:
    AetherProcessor();
    ~AetherProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AETHER"; }
    bool acceptsMidi() const override  { return true; }
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
    juce::MidiKeyboardState keyboardState;
    rf::ModMatrix modMatrix;
    aether::Stack stack;

private:
    juce::Synthesiser synth;
    juce::dsp::LadderFilter<float> filter;

    void updateFilter();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AetherProcessor)
};
