#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

class SpectroEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpectroEditor (SpectroProcessor&);
    ~SpectroEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setFftSize (int size);
    void addColumns();
    float yOf (float freq) const;

    SpectroProcessor& proc;
    juce::ComboBox resBox;
    juce::ToggleButton notesBtn { "Every note line" };

    juce::Image history;
    juce::Rectangle<int> plot;

    std::unique_ptr<juce::dsp::FFT> fft;
    int fftSize = 0;
    std::vector<float> fftData, window, bands;
    std::array<juce::Colour, 256> lut;

    float peakF = 0.0f, peakV = 0.0f;
    bool showNotes = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectroEditor)
};
