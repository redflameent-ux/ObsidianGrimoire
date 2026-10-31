#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "BandData.h"
#include "../redflame/RedFlameMod.h"

// CLEANSE - mastering / dynamic EQ (effect). A series of biquad bands built from the
// EqModel, plus drive + output + ceiling.
class CleanseProcessor : public juce::AudioProcessor
{
public:
    CleanseProcessor();
    ~CleanseProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CLEANSE"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Open Air"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    rf::ModMatrix modMatrix;
    cleanse::EqModel eq;
    std::atomic<float> outLevel { 0.0f };

private:
    double sr = 44100.0;
    juce::dsp::IIR::Filter<float> filters[2][cleanse::kBands];
    void updateCoeffs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanseProcessor)
};
