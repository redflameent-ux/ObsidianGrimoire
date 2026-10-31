#include "PluginProcessor.h"
#include "PluginEditor.h"

// ---------------------------------------------------------------- synth voice + sound
namespace
{
    struct OracleSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override    { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    struct OracleVoice : public juce::SynthesiserVoice
    {
        explicit OracleVoice (juce::AudioProcessorValueTreeState& s) : state (s)
        {
            aLevel = state.getRawParameterValue (pid::aLevel);
            bLevel = state.getRawParameterValue (pid::bLevel);
            bNoise = state.getRawParameterValue (pid::bNoise);
            aOct = state.getRawParameterValue (pid::aOct);  aSemi = state.getRawParameterValue (pid::aSemi);
            aFine = state.getRawParameterValue (pid::aFine);
            bOct = state.getRawParameterValue (pid::bOct);  bSemi = state.getRawParameterValue (pid::bSemi);
            bFine = state.getRawParameterValue (pid::bFine);
            e1a = state.getRawParameterValue (envId (1, "a")); e1d = state.getRawParameterValue (envId (1, "d"));
            e1s = state.getRawParameterValue (envId (1, "s")); e1r = state.getRawParameterValue (envId (1, "r"));
            tune = state.getRawParameterValue (pid::vTune);
        }

        bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<OracleSound*> (s) != nullptr; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            note = midiNote; level = velocity;
            phaseA = phaseB = 0.0;
            adsr.setSampleRate (getSampleRate());
            adsr.setParameters ({ juce::jmax (0.001f, e1a->load() * 0.001f),
                                  juce::jmax (0.001f, e1d->load() * 0.001f),
                                  juce::jlimit (0.0f, 1.0f, e1s->load() * 0.01f),
                                  juce::jmax (0.005f, e1r->load() * 0.001f) });
            adsr.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff) adsr.noteOff();
            else { adsr.reset(); clearCurrentNote(); }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
        {
            if (! adsr.isActive()) return;

            const double sr = getSampleRate();
            const double base = juce::MidiMessage::getMidiNoteInHertz (note) * std::pow (2.0, tune->load() / 1200.0);
            const double fA = base * std::pow (2.0, (aOct->load() * 12.0 + aSemi->load() + aFine->load() * 0.01) / 12.0);
            const double fB = base * std::pow (2.0, (bOct->load() * 12.0 + bSemi->load() + bFine->load() * 0.01) / 12.0);
            const float gA = aLevel->load() * 0.01f;
            const float gB = bLevel->load() * 0.01f;
            const float gN = bNoise->load() * 0.01f;
            const double incA = fA / sr, incB = fB / sr;

            for (int i = 0; i < num; ++i)
            {
                const float saw = 2.0f * (float) phaseA - 1.0f;
                const float sine = std::sin (juce::MathConstants<float>::twoPi * (float) phaseB);
                const float noise = rng.nextFloat() * 2.0f - 1.0f;
                float s = (saw * gA + sine * gB + noise * gN) * 0.3f;
                s *= adsr.getNextSample() * level;

                out.addSample (0, start + i, s);
                if (out.getNumChannels() > 1) out.addSample (1, start + i, s);

                phaseA += incA; if (phaseA >= 1.0) phaseA -= 1.0;
                phaseB += incB; if (phaseB >= 1.0) phaseB -= 1.0;
            }
            if (! adsr.isActive()) clearCurrentNote();
        }

        juce::AudioProcessorValueTreeState& state;
        juce::ADSR adsr;
        juce::Random rng;
        double phaseA = 0.0, phaseB = 0.0;
        int note = 60; float level = 1.0f;
        std::atomic<float>* aLevel; std::atomic<float>* bLevel; std::atomic<float>* bNoise;
        std::atomic<float>* aOct; std::atomic<float>* aSemi; std::atomic<float>* aFine;
        std::atomic<float>* bOct; std::atomic<float>* bSemi; std::atomic<float>* bFine;
        std::atomic<float>* e1a; std::atomic<float>* e1d; std::atomic<float>* e1s; std::atomic<float>* e1r;
        std::atomic<float>* tune;
    };
}

// ---------------------------------------------------------------- processor
OracleProcessor::OracleProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Oracle", OracleLayout())
{
    synth.addSound (new OracleSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice (new OracleVoice (apvts));
}

void OracleProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    filter.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    filter.setEnabled (true);
    updateFilter();
}

bool OracleProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OracleProcessor::updateFilter()
{
    using M = juce::dsp::LadderFilterMode;
    switch ((int) *apvts.getRawParameterValue (pid::fType))
    {
        case 1:  filter.setMode (M::LPF12); break;
        case 2:  filter.setMode (M::BPF12); break;
        case 3:  filter.setMode (M::HPF12); break;
        default: filter.setMode (M::LPF24); break;
    }
    filter.setCutoffFrequencyHz (juce::jlimit (20.0f, 20000.0f, apvts.getRawParameterValue (pid::fCutoff)->load()));
    filter.setResonance (juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue (pid::fReso)->load() * 0.0098f));
    filter.setDrive (1.0f + apvts.getRawParameterValue (pid::fDrive)->load() * 0.03f);
}

void OracleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    updateFilter();
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    filter.process (ctx);

    const float gain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (pid::master)->load());
    buffer.applyGain (gain);
}

juce::AudioProcessorEditor* OracleProcessor::createEditor() { return new OracleEditor (*this); }

void OracleProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void OracleProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OracleProcessor(); }
