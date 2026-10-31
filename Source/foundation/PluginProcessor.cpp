#include "PluginProcessor.h"
#include "PluginEditor.h"

FoundationProcessor::FoundationProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("In",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "FOUNDATION", foundationLayout())
{
}

void FoundationProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate; compEnv = limEnv = 0.0f; inRms = outRms = 0.0f;
}

bool FoundationProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

void FoundationProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    if (P (fnd::bypass) > 0.5f) { reduceDb.store (0.0f); limitDb.store (0.0f); return; }

    const float inG  = juce::Decibels::decibelsToGain (P (fnd::input));
    const float thick = P (fnd::thick) * 0.01f, color = P (fnd::color) * 0.01f;
    const float comp = P (fnd::comp) * 0.01f, crunch = P (fnd::crunch) * 0.01f;
    const bool  ott  = (int) P (fnd::compMode) == 1;
    const bool  clip = P (fnd::clip) > 0.5f;
    const float loud = P (fnd::loud) * 0.01f, width = P (fnd::width) * 0.01f;
    const float wet  = P (fnd::mix) * 0.01f;
    const float outG = juce::Decibels::decibelsToGain (P (fnd::output) + P (fnd::master));

    const float drive = 1.0f + thick * 5.0f + std::abs (color) * 3.0f;
    const float thr = -6.0f - comp * 34.0f;               // compressor threshold (dB)
    const float ratio = ott ? 6.0f : 4.0f;
    const float attC = std::exp (-1.0f / (0.005f * (float) sr));
    const float relC = std::exp (-1.0f / (0.120f * (float) sr));
    const float limCeil = juce::Decibels::decibelsToGain (-0.5f);

    float* Lp = buffer.getWritePointer (0);
    float* Rp = ch > 1 ? buffer.getWritePointer (1) : nullptr;

    float inAcc = 0.0f, outAcc = 0.0f, maxGr = 0.0f, maxLim = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float l = Lp[i] * inG;
        float r = Rp ? Rp[i] * inG : l;
        const float dryL = l, dryR = r;
        inAcc += l * l + r * r;

        // saturate
        l = std::tanh (l * drive + color * 0.25f);
        r = std::tanh (r * drive + color * 0.25f);

        // compress (linked peak)
        const float peak = juce::jmax (std::abs (l), std::abs (r));
        compEnv = peak > compEnv ? peak + (compEnv - peak) * attC : peak + (compEnv - peak) * relC;
        const float envDb = juce::Decibels::gainToDecibels (compEnv + 1.0e-6f);
        float gr = envDb > thr ? (envDb - thr) * (1.0f - 1.0f / ratio) : 0.0f;
        const float cg = juce::Decibels::decibelsToGain (-gr);
        l *= cg; r *= cg; maxGr = juce::jmax (maxGr, gr);

        // clip / crunch
        if (clip) { l = std::tanh (l * (1.0f + crunch * 4.0f)); r = std::tanh (r * (1.0f + crunch * 4.0f)); }

        // loudness push
        const float lg = 1.0f + loud * 2.0f;
        l *= lg; r *= lg;

        // width (mid/side)
        if (Rp) { const float m = (l + r) * 0.5f, s = (l - r) * 0.5f * (0.2f + width * 0.8f); l = m + s; r = m - s; }

        // limiter (soft)
        const float lp = juce::jmax (std::abs (l), std::abs (r));
        limEnv = lp > limEnv ? lp : lp + (limEnv - lp) * relC;
        if (limEnv > limCeil)
        {
            const float g = limCeil / (limEnv + 1.0e-6f);
            l *= g; r *= g; maxLim = juce::jmax (maxLim, juce::Decibels::gainToDecibels (g) * -1.0f);
        }

        // mix + output
        l = (dryL + (l - dryL) * wet) * outG;
        r = (dryR + (r - dryR) * wet) * outG;
        outAcc += l * l + r * r;

        Lp[i] = l; if (Rp) Rp[i] = r;
    }

    const float inR = std::sqrt (inAcc / juce::jmax (1, n * 2));
    const float outR = std::sqrt (outAcc / juce::jmax (1, n * 2));
    inRms += (inR - inRms) * 0.2f; outRms += (outR - outRms) * 0.2f;
    inLufs.store (juce::Decibels::gainToDecibels (inRms + 1.0e-6f) - 3.0f);
    outLufs.store (juce::Decibels::gainToDecibels (outRms + 1.0e-6f) - 3.0f);
    reduceDb.store (reduceDb.load() + (maxGr - reduceDb.load()) * 0.3f);
    limitDb.store (limitDb.load() + (maxLim - limitDb.load()) * 0.3f);
}

juce::AudioProcessorEditor* FoundationProcessor::createEditor() { return new FoundationEditor (*this); }

void FoundationProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void FoundationProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FoundationProcessor(); }
