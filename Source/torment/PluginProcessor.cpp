#include "PluginProcessor.h"
#include "PluginEditor.h"

TormentProcessor::TormentProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("In",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TORMENT", tormentLayout())
{
}

void TormentProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (int c = 0; c < 2; ++c) { sh[c] = 0.0f; shCount[c] = 0.0f; lp[c] = 0.0f; }
}

bool TormentProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

void TormentProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float tapeAmt = P (tid::tapeAmt) * 0.01f, decAmt = P (tid::decAmt) * 0.01f, vinAmt = P (tid::vinAmt) * 0.01f;
    const float tDrive = P (tid::tDrive) * 0.01f, tHiss = P (tid::tHiss) * 0.01f;
    const float dRate = P (tid::dRate) * 0.01f, dBits = P (tid::dBits) * 0.01f;
    const float vDust = P (tid::vDust) * 0.01f, vPops = P (tid::vPops) * 0.01f, vRumble = P (tid::vRumble) * 0.01f, vTone = P (tid::vTone) * 0.01f;
    const float wet = P (tid::mix) * 0.01f;

    const float holdLen = 1.0f + dRate * 40.0f;
    const float bitsN = 16.0f - dBits * 14.0f;
    const float levels = std::pow (2.0f, juce::jmax (1.0f, bitsN));
    const float toneCoef = juce::jlimit (0.02f, 1.0f, 0.05f + vTone * 0.95f);  // higher tone = brighter

    float peak = 0.0f;
    for (int c = 0; c < juce::jmin (ch, 2); ++c)
    {
        auto* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float dry = d[i];
            float s = dry;

            // tape: drive + hiss
            {
                float t = std::tanh (s * (1.0f + tDrive * 6.0f));
                t += (rng.nextFloat() - 0.5f) * tHiss * 0.03f;
                s = s + (t - s) * tapeAmt;
            }
            // decimate: sample-rate + bit crush
            {
                shCount[c] += 1.0f;
                if (shCount[c] >= holdLen) { shCount[c] -= holdLen; sh[c] = s; }
                float q = std::round (sh[c] * levels) / levels;
                s = s + (q - s) * decAmt;
            }
            // vinyl: dust/pops/rumble + tone
            {
                float v = s;
                if (rng.nextFloat() < 0.0008f * (1.0f + vPops * 20.0f)) v += (rng.nextFloat() * 2.0f - 1.0f) * vPops * 0.6f;
                v += (rng.nextFloat() - 0.5f) * vDust * 0.02f;
                v += std::sin (i * 0.0007f) * vRumble * 0.05f;
                lp[c] += toneCoef * (v - lp[c]);
                v = lp[c];
                s = s + (v - s) * vinAmt;
            }

            d[i] = dry + (s - dry) * wet;
            peak = juce::jmax (peak, std::abs (d[i]));
        }
    }
    scopeLevel.store (peak);

    buffer.applyGain (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (tid::master)->load()));
}

juce::AudioProcessorEditor* TormentProcessor::createEditor() { return new TormentEditor (*this); }

void TormentProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void TormentProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TormentProcessor(); }
