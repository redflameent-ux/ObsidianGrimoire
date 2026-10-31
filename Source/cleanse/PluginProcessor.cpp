#include "PluginProcessor.h"
#include "PluginEditor.h"

CleanseProcessor::CleanseProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("In",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CLEANSE", cleanseLayout())
{
}

void CleanseProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    for (int ch = 0; ch < 2; ++ch)
        for (int b = 0; b < cleanse::kBands; ++b) { filters[ch][b].prepare (spec); filters[ch][b].reset(); }
    updateCoeffs();
}

bool CleanseProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

void CleanseProcessor::updateCoeffs()
{
    using C = juce::dsp::IIR::Coefficients<float>;
    for (int b = 0; b < cleanse::kBands; ++b)
    {
        auto& band = eq.bands[b];
        const float fr = juce::jlimit (20.0f, 20000.0f, band.freq.load());
        const float q  = juce::jlimit (0.1f, 10.0f, band.q.load());
        const float g  = juce::Decibels::decibelsToGain (band.gain.load());
        juce::ReferenceCountedObjectPtr<C> co;
        switch (band.type.load())
        {
            case 1:  co = C::makeHighPass (sr, fr, q); break;
            case 2:  co = C::makeLowPass  (sr, fr, q); break;
            case 3:  co = C::makeLowShelf (sr, fr, q, g); break;
            case 4:  co = C::makeHighShelf (sr, fr, q, g); break;
            default: co = C::makePeakFilter (sr, fr, q, g); break;
        }
        filters[0][b].coefficients = co;
        filters[1][b].coefficients = co;
    }
}

void CleanseProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    updateCoeffs();
    const float drive = P (cid::drive) * 0.01f;
    const float wet = P (cid::mix) * 0.01f;
    const float outG = juce::Decibels::decibelsToGain (P (cid::output) + P (cid::master));
    const float ceil = juce::Decibels::decibelsToGain (P (cid::ceiling));
    const bool lim = P (cid::limiter) > 0.5f;

    float peak = 0.0f;
    for (int c = 0; c < juce::jmin (ch, 2); ++c)
    {
        auto* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float dry = d[i];
            float s = dry;
            if (drive > 0.001f) s = std::tanh (s * (1.0f + drive * 3.0f));
            for (int b = 0; b < cleanse::kBands; ++b)
                if (eq.bands[b].on.load()) s = filters[c][b].processSample (s);
            s = dry + (s - dry) * wet;
            s *= outG;
            if (lim && std::abs (s) > ceil) s = ceil * std::tanh (s / ceil);
            d[i] = s;
            peak = juce::jmax (peak, std::abs (s));
        }
    }
    outLevel.store (outLevel.load() + (peak - outLevel.load()) * 0.2f);
}

juce::AudioProcessorEditor* CleanseProcessor::createEditor() { return new CleanseEditor (*this); }

void CleanseProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    state.removeChild (state.getChildWithName ("EQ"), nullptr);
    state.appendChild (modMatrix.toTree(), nullptr);
    state.appendChild (eq.toTree(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void CleanseProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        modMatrix.fromTree (tree.getChildWithName ("MODMATRIX"));
        eq.fromTree (tree.getChildWithName ("EQ"));
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CleanseProcessor(); }
