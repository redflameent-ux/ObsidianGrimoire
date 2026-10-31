#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    struct AetherSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override    { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    struct AetherVoice : public juce::SynthesiserVoice
    {
        AetherVoice (juce::AudioProcessorValueTreeState& s, aether::Stack& st) : state (s), stack (st)
        {
            eA = s.getRawParameterValue (aid::eA); eD = s.getRawParameterValue (aid::eD);
            eS = s.getRawParameterValue (aid::eS); eR = s.getRawParameterValue (aid::eR);
        }
        bool canPlaySound (juce::SynthesiserSound* snd) override { return dynamic_cast<AetherSound*> (snd) != nullptr; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            note = midiNote; level = velocity;
            for (auto& p : phase) p = 0.0;
            adsr.setSampleRate (getSampleRate());
            adsr.setParameters ({ juce::jmax (0.001f, eA->load() * 0.001f), juce::jmax (0.001f, eD->load() * 0.001f),
                                  juce::jlimit (0.0f, 1.0f, eS->load() * 0.01f), juce::jmax (0.005f, eR->load() * 0.001f) });
            adsr.noteOn();
        }
        void stopNote (float, bool tail) override { if (tail) adsr.noteOff(); else { adsr.reset(); clearCurrentNote(); } }
        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        static float osc (int wave, double ph, juce::Random& rng)
        {
            const float p = (float) ph;
            switch (wave)
            {
                case 0: return 2.0f * p - 1.0f;                                           // saw
                case 1: return p < 0.5f ? 1.0f : -1.0f;                                   // square
                case 2: return std::sin (p * juce::MathConstants<float>::twoPi);          // sine
                case 3: return 1.0f - 4.0f * std::abs (p - 0.5f);                         // tri
                default: return rng.nextFloat() * 2.0f - 1.0f;                            // noise
            }
        }

        void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
        {
            if (! adsr.isActive()) return;
            const double sr = getSampleRate();
            const double base = juce::MidiMessage::getMidiNoteInHertz (note);

            for (int i = 0; i < num; ++i)
            {
                float mix = 0.0f;
                const float env = adsr.getNextSample();
                for (int l = 0; l < aether::kLayers; ++l)
                {
                    auto& ly = stack.layers[l];
                    if (! ly.on.load()) continue;
                    const double freq = base * std::pow (2.0, ly.coarse.load() / 12.0) * std::pow (2.0, ly.fine.load() / 1200.0);
                    phase[l] += freq / sr; if (phase[l] >= 1.0) phase[l] -= 1.0;
                    mix += osc (ly.wave.load(), phase[l], rng) * (ly.level.load() * 0.01f);
                }
                mix *= env * level * 0.18f;
                out.addSample (0, start + i, mix);
                if (out.getNumChannels() > 1) out.addSample (1, start + i, mix);
            }
            if (! adsr.isActive()) clearCurrentNote();
        }

        juce::AudioProcessorValueTreeState& state;
        aether::Stack& stack;
        juce::ADSR adsr; juce::Random rng;
        double phase[aether::kLayers] = { 0.0 };
        int note = 60; float level = 1.0f;
        std::atomic<float>* eA; std::atomic<float>* eD; std::atomic<float>* eS; std::atomic<float>* eR;
    };
}

AetherProcessor::AetherProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AETHER", aetherLayout())
{
    synth.addSound (new AetherSound());
    for (int i = 0; i < 16; ++i) synth.addVoice (new AetherVoice (apvts, stack));
}

void AetherProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    filter.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    filter.setEnabled (true);
}

bool AetherProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& o = layouts.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

void AetherProcessor::updateFilter()
{
    using M = juce::dsp::LadderFilterMode;
    switch ((int) *apvts.getRawParameterValue (aid::fAtype))
    {
        case 1:  filter.setMode (M::HPF24); break;
        case 2:  filter.setMode (M::BPF24); break;
        default: filter.setMode (M::LPF24); break;
    }
    filter.setCutoffFrequencyHz (juce::jlimit (20.0f, 20000.0f, apvts.getRawParameterValue (aid::fAcut)->load()));
    filter.setResonance (juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue (aid::fAres)->load() * 0.0098f));
    filter.setDrive (1.0f + apvts.getRawParameterValue (aid::fAdrive)->load() * 0.03f);
}

void AetherProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    if (apvts.getRawParameterValue (aid::fAon)->load() > 0.5f)
    {
        updateFilter();
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filter.process (ctx);
    }

    buffer.applyGain (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (aid::master)->load()));
}

juce::AudioProcessorEditor* AetherProcessor::createEditor() { return new AetherEditor (*this); }

void AetherProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.removeChild (state.getChildWithName ("STACK"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    state.appendChild (stack.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void AetherProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        stack.fromTree (tree.getChildWithName ("STACK"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AetherProcessor(); }
