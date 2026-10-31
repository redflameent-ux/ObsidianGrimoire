#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "../redflame/RedFlameMod.h"

// ORACLE (RedFlame UI kit proof). A deliberately small synth voice behind the real UI:
// two oscillators (saw + sine) plus noise through a ladder filter and master gain.
// It exists to validate the shared kit end to end; the full wavetable engine is later.
class OracleProcessor : public juce::AudioProcessor
{
public:
    OracleProcessor();
    ~OracleProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ORACLE"; }
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

private:
    juce::Synthesiser synth;
    juce::dsp::LadderFilter<float> filter;
    double sr = 44100.0;

    void updateFilter();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OracleProcessor)
};
