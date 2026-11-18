#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

class SpectroProcessor : public juce::AudioProcessor
{
public:
    SpectroProcessor();

    void prepareToPlay (double sr, int) override { sampleRate = sr; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == l.getMainInputChannelSet()
            && ! l.getMainOutputChannelSet().isDisabled();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Octave Spectrogram"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& d) override
    {
        juce::MemoryOutputStream s (d, true);
        s.writeInt (resBands.load());
    }
    void setStateInformation (const void* data, int size) override
    {
        if (size >= 4)
        {
            juce::MemoryInputStream s (data, (size_t) size, false);
            const int v = s.readInt();
            if (v == 512 || v == 1024 || v == 2048 || v == 4096) resBands = v;
        }
    }

    static constexpr int ringSize = 1 << 16;
    std::array<float, ringSize> ring {};
    std::atomic<int> writePos { 0 };
    std::atomic<int> resBands { 4096 };
    std::atomic<double> sampleRateAtomic { 44100.0 };
    double sampleRate = 44100.0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectroProcessor)
};
