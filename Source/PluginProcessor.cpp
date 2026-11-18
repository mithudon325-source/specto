#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectroProcessor::SpectroProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void SpectroProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    sampleRateAtomic.store (getSampleRate() > 0 ? getSampleRate() : 44100.0);

    const int n  = b.getNumSamples();
    const int nc = b.getNumChannels();
    int w = writePos.load (std::memory_order_relaxed);

    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < nc; ++c)
            s += b.getReadPointer (c)[i];
        ring[(size_t) w] = s / (float) juce::jmax (1, nc);
        w = (w + 1) & (ringSize - 1);
    }
    writePos.store (w, std::memory_order_release);
    // audio passes through untouched
}

juce::AudioProcessorEditor* SpectroProcessor::createEditor() { return new SpectroEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpectroProcessor(); }
