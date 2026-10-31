#include "PluginProcessor.h"
#include "PluginEditor.h"

IncantProcessor::IncantProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "INCANT", incantLayout())
{
}

void IncantProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    stepPhase = 0.0; step = 0; prevRun = false;
    for (auto& d : drums) d.active = false;
}

bool IncantProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& o = layouts.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

void IncantProcessor::triggerStep (int s)
{
    const float energy = apvts.getRawParameterValue (fid::energy)->load();
    const float accent = apvts.getRawParameterValue (fid::accent)->load();
    for (int l = 0; l < incant::kLanes; ++l)
    {
        if (pattern.mute[l].load()) continue;
        const float v = pattern.get (l, s);
        if (v <= 0.0f) continue;
        float eff = v * (0.4f + energy * 0.009f) * pattern.level[l].load();
        if (accent > 0.0f && (s % 4) == 0) eff *= 1.0f + accent * 0.005f;
        drums[l].trigger (incant::LANES[l].type, juce::jlimit (0.0f, 1.2f, eff), pattern.pan[l].load());
    }
}

void IncantProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    buffer.clear();

    double bpm = 120.0; bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostPlaying = pos->getIsPlaying();
        }

    const bool run = apvts.getRawParameterValue (fid::play)->load() > 0.5f || hostPlaying;
    const double base = (60.0 / juce::jmax (20.0, bpm)) / 4.0 * sr;   // samples per 16th
    const float swingAmt = (apvts.getRawParameterValue (fid::swing)->load() - 50.0f) * 0.012f;
    auto durFor = [&] (int s) { return base * (1.0 + (s % 2 == 0 ? swingAmt : -swingAmt)); };

    if (run && ! prevRun) { step = 0; stepPhase = 0.0; stepDur = durFor (0); triggerStep (0); }
    prevRun = run;
    uiPlaying.store (run);

    float* L = buffer.getWritePointer (0);
    float* R = ch > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        if (run)
        {
            stepPhase += 1.0;
            if (stepPhase >= stepDur)
            {
                stepPhase -= stepDur;
                step = (step + 1) % incant::kSteps;
                triggerStep (step);
                stepDur = durFor (step);
            }
        }
        float l = 0.0f, r = 0.0f;
        for (auto& d : drums)
        {
            const float s = d.next (sr);
            const float p = juce::jlimit (-1.0f, 1.0f, d.pan);
            l += s * (0.5f - p * 0.5f);
            r += s * (0.5f + p * 0.5f);
        }
        L[i] += l;
        if (R) R[i] += r;
    }

    uiStep.store (run ? step : -1);

    buffer.applyGain (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (fid::master)->load()));
}

void IncantProcessor::generate (int genre)
{
    juce::Random rng;
    pattern.clearAll();
    // kick
    pattern.set (0, 0, 0.95f); pattern.set (0, 8, 0.9f);
    if (genre != 1) { pattern.set (0, 4, 0.85f); pattern.set (0, 12, 0.85f); } // not minimal
    if (rng.nextFloat() < 0.5f) pattern.set (0, 10 + rng.nextInt (4), 0.6f);
    // snare / clap on 2 & 4
    pattern.set (1, 4, 0.9f); pattern.set (1, 12, 0.9f);
    if (genre == 0 || genre == 4) { pattern.set (4, 4, 0.7f); pattern.set (4, 12, 0.7f); }
    // closed hats
    for (int s = 0; s < incant::kSteps; ++s)
        if ((s % 2 == 0) || rng.nextFloat() < (genre == 4 ? 0.5f : 0.2f))
            pattern.set (2, s, 0.4f + rng.nextFloat() * 0.4f);
    // open hats on offbeats
    pattern.set (3, 2, 0.5f); pattern.set (3, 10, 0.5f);
    // perc flavour
    for (int s = 0; s < incant::kSteps; ++s)
    {
        if (rng.nextFloat() < 0.12f) pattern.set (5, s, 0.5f + rng.nextFloat() * 0.3f);
        if (rng.nextFloat() < 0.15f) pattern.set (7, s, 0.4f + rng.nextFloat() * 0.3f);
        if (rng.nextFloat() < 0.08f) pattern.set (6, s, 0.5f);
    }
}

juce::AudioProcessorEditor* IncantProcessor::createEditor() { return new IncantEditor (*this); }

void IncantProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.removeChild (state.getChildWithName ("PATTERN"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    state.appendChild (pattern.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void IncantProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        pattern.fromTree (tree.getChildWithName ("PATTERN"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new IncantProcessor(); }
