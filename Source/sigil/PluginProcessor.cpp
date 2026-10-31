#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    struct SigilSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override    { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    // Karplus-Strong resonator voice.
    struct SigilVoice : public juce::SynthesiserVoice
    {
        explicit SigilVoice (juce::AudioProcessorValueTreeState& s) : state (s)
        {
            strike = s.getRawParameterValue (sid::strike);   structure = s.getRawParameterValue (sid::structure);
            bright = s.getRawParameterValue (sid::bright);    damping   = s.getRawParameterValue (sid::damping);
            position = s.getRawParameterValue (sid::position);
            octave = s.getRawParameterValue (sid::octave);    tune = s.getRawParameterValue (sid::tune);
            atk = s.getRawParameterValue (sid::atk); dec = s.getRawParameterValue (sid::dec);
            sus = s.getRawParameterValue (sid::sus); rel = s.getRawParameterValue (sid::rel);
            buf.resize (4096, 0.0f);
        }

        bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<SigilSound*> (s) != nullptr; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            const double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote)
                                * std::pow (2.0, (octave->load() * 12.0 + tune->load() * 0.01) / 12.0);
            N = juce::jlimit (2, (int) buf.size() - 2, (int) std::round (getSampleRate() / juce::jmax (20.0, freq)));
            std::fill (buf.begin(), buf.end(), 0.0f);
            idx = 0;
            level = velocity;
            // excitation length scales with structure (longer = more "bowed")
            exciteLeft = juce::jmax (1, (int) (N * (0.5f + structure->load() * 0.01f)));
            adsr.setSampleRate (getSampleRate());
            adsr.setParameters ({ juce::jmax (0.001f, atk->load() * 0.001f),
                                  juce::jmax (0.001f, dec->load() * 0.001f),
                                  juce::jlimit (0.0f, 1.0f, sus->load() * 0.01f),
                                  juce::jmax (0.005f, rel->load() * 0.001f) });
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
            const float strk = strike->load() * 0.01f;
            const float br   = bright->load() * 0.01f;                              // 0 dark .. 1 bright
            const float dmp  = juce::jmap (damping->load(), 0.0f, 100.0f, 0.999f, 0.90f);
            const int   posN = juce::jlimit (0, N - 1, (int) (position->load() * 0.01f * N * 0.5f));

            for (int i = 0; i < num; ++i)
            {
                float x = 0.0f;
                if (exciteLeft > 0) { x = (rng.nextFloat() * 2.0f - 1.0f) * strk; --exciteLeft; }

                const float cur = buf[(size_t) idx];
                const int nx = (idx + 1) % N;
                const float avg = 0.5f * (cur + buf[(size_t) nx]);
                const float filtered = avg + br * (cur - avg);          // brightness blend
                buf[(size_t) idx] = x + dmp * filtered;

                const float posS = buf[(size_t) ((idx + posN) % N)];
                float s = (cur - 0.5f * position->load() * 0.01f * posS) * adsr.getNextSample() * level * 0.6f;

                out.addSample (0, start + i, s);
                if (out.getNumChannels() > 1) out.addSample (1, start + i, s);
                idx = nx;
            }
            if (! adsr.isActive()) clearCurrentNote();
        }

        juce::AudioProcessorValueTreeState& state;
        juce::ADSR adsr;
        juce::Random rng;
        std::vector<float> buf;
        int N = 2, idx = 0, exciteLeft = 0;
        float level = 1.0f;
        std::atomic<float>* strike; std::atomic<float>* structure; std::atomic<float>* bright;
        std::atomic<float>* damping; std::atomic<float>* position;
        std::atomic<float>* octave; std::atomic<float>* tune;
        std::atomic<float>* atk; std::atomic<float>* dec; std::atomic<float>* sus; std::atomic<float>* rel;
    };
}

SigilProcessor::SigilProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SIGIL", sigilLayout())
{
    synth.addSound (new SigilSound());
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new SigilVoice (apvts));
}

void SigilProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    reverb.setSampleRate (sampleRate);
    juce::ignoreUnused (samplesPerBlock);
}

bool SigilProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SigilProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    if (apvts.getRawParameterValue (sid::reverbOn)->load() > 0.5f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue (sid::reverbTime)->load() * 0.01f);
        rp.wetLevel = juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue (sid::reverbMix)->load() * 0.01f);
        rp.dryLevel = 1.0f - rp.wetLevel * 0.5f;
        rp.damping  = 1.0f - juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue (sid::reverbTone)->load() * 0.01f);
        rp.width    = 1.0f;
        reverb.setParameters (rp);
        if (buffer.getNumChannels() >= 2)
            reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
        else
            reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
    }

    buffer.applyGain (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (sid::master)->load()));
}

juce::AudioProcessorEditor* SigilProcessor::createEditor() { return new SigilEditor (*this); }

void SigilProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void SigilProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SigilProcessor(); }
