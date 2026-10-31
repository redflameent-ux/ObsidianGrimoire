#include "PluginProcessor.h"
#include "PluginEditor.h"

EtherealProcessor::EtherealProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("In",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ETHEREAL", etherealLayout())
{
}

void EtherealProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    preDelay.prepare (spec);
    preDelay.setMaximumDelayInSamples ((int) (0.25 * sampleRate) + 4);
    loCut.prepare (spec); loCut.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    hiCut.prepare (spec); hiCut.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    reverb.setSampleRate (sampleRate);
    dryBuf.setSize (2, samplesPerBlock, false, false, true);
}

bool EtherealProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void EtherealProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();

    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // keep the dry signal
    dryBuf.setSize (ch, n, false, false, true);
    for (int c = 0; c < ch; ++c) dryBuf.copyFrom (c, 0, buffer, c, 0, n);

    // input gain + drive soft-clip
    const float inG   = juce::Decibels::decibelsToGain (p (eid::inGain));
    const float drive = p (eid::drive) * 0.01f;
    buffer.applyGain (inG);
    if (drive > 0.0001f)
        for (int c = 0; c < ch; ++c)
        {
            auto* d = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i) d[i] = std::tanh (d[i] * (1.0f + drive * 4.0f));
        }

    // pre-reverb filtering + pre-delay
    loCut.setCutoffFrequency (juce::jlimit (20.0f, 2000.0f, p (eid::loCut)));
    hiCut.setCutoffFrequency (juce::jlimit (1000.0f, 20000.0f, p (eid::hiCut)));
    preDelay.setDelay (juce::jlimit (0.0f, (float) (0.25 * sr), p (eid::preDelay) * 0.001f * (float) sr));
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        loCut.process (ctx); hiCut.process (ctx); preDelay.process (ctx);
    }

    // reverb
    juce::Reverb::Parameters rp;
    rp.roomSize   = juce::jlimit (0.1f, 0.98f, juce::jmap (p (eid::decay), 0.1f, 20.0f, 0.30f, 0.97f));
    rp.damping    = juce::jlimit (0.0f, 1.0f, juce::jmap (p (eid::damp), 200.0f, 18000.0f, 0.95f, 0.05f));
    rp.width      = juce::jlimit (0.0f, 1.0f, p (eid::width) * 0.01f);
    rp.wetLevel   = 1.0f; rp.dryLevel = 0.0f;
    rp.freezeMode = p (eid::freeze) > 0.5f ? 1.0f : 0.0f;
    reverb.setParameters (rp);
    if (ch >= 2) reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    else         reverb.processMono   (buffer.getWritePointer (0), n);

    // dry / wet
    const float wet = p (eid::mix) * 0.01f, dry = 1.0f - wet;
    for (int c = 0; c < ch; ++c)
    {
        auto* o = buffer.getWritePointer (c);
        auto* dr = dryBuf.getReadPointer (c);
        for (int i = 0; i < n; ++i) o[i] = o[i] * wet + dr[i] * dry;
    }

    // output: makeup + master, optional limit
    const float out = juce::Decibels::decibelsToGain (p (eid::makeup) + p (eid::master));
    buffer.applyGain (out);
    if (p (eid::limit) > 0.5f)
        for (int c = 0; c < ch; ++c)
        {
            auto* o = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i) o[i] = std::tanh (o[i]);
        }
}

juce::AudioProcessorEditor* EtherealProcessor::createEditor() { return new EtherealEditor (*this); }

void EtherealProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void EtherealProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EtherealProcessor(); }
