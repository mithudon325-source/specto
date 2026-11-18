#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float FMIN = 20.0f, FMAX = 20000.0f;
    constexpr float DBMIN = -100.0f, DBMAX = -10.0f;
    constexpr int   KEYW = 44, AXW = 62, TOPBAR = 32;
    const float LOGR = std::log (FMAX / FMIN);

    float noteFreq (int n) { return 440.0f * std::pow (2.0f, (float) (n - 69) / 12.0f); }

    juce::String noteName (float f)
    {
        static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        const int n = (int) std::lround (12.0 * std::log2 (f / 440.0) + 69.0);
        return juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
    }
}

SpectroEditor::SpectroEditor (SpectroProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    // black > purple > magenta > orange > yellow > white
    struct Stop { float v; juce::uint8 r, g, b; };
    const Stop st[] = { {0,0,0,0}, {.22f,35,0,70}, {.45f,160,0,110}, {.68f,255,80,20}, {.86f,255,200,40}, {1,255,255,225} };
    for (int i = 0; i < 256; ++i)
    {
        const float v = (float) i / 255.0f;
        int k = 1;
        while (k < 5 && v > st[k].v) ++k;
        const float t = (v - st[k-1].v) / (st[k].v - st[k-1].v);
        auto mix = [&] (juce::uint8 a, juce::uint8 b) { return (juce::uint8) juce::jlimit (0.0f, 255.0f, a + (b - a) * t); };
        lut[(size_t) i] = juce::Colour (mix (st[k-1].r, st[k].r), mix (st[k-1].g, st[k].g), mix (st[k-1].b, st[k].b));
    }

    resBox.addItem ("512 bands", 1);
    resBox.addItem ("1024 bands", 2);
    resBox.addItem ("2048 bands", 3);
    resBox.addItem ("4096 bands", 4);
    const int cur = proc.resBands.load();
    resBox.setSelectedId (cur == 512 ? 1 : cur == 1024 ? 2 : cur == 2048 ? 3 : 4, juce::dontSendNotification);
    resBox.onChange = [this]
    {
        static const int v[] = { 0, 512, 1024, 2048, 4096 };
        proc.resBands = v[resBox.getSelectedId()];
    };
    addAndMakeVisible (resBox);

    notesBtn.onClick = [this] { showNotes = notesBtn.getToggleState(); };
    addAndMakeVisible (notesBtn);

    setFftSize (proc.resBands.load() * 8);
    setResizable (true, true);
    setResizeLimits (320, 420, 1400, 1800);
    setSize (480, 740);
    startTimerHz (60);
}

void SpectroEditor::setFftSize (int size)
{
    fftSize = size;
    fft = std::make_unique<juce::dsp::FFT> (juce::findHighestSetBit (size));
    fftData.assign ((size_t) size * 2, 0.0f);
    window.assign ((size_t) size, 0.0f);
    juce::dsp::WindowingFunction<float>::fillWindowingTables (window.data(), (size_t) size,
        juce::dsp::WindowingFunction<float>::hann, false);
}

void SpectroEditor::resized()
{
    resBox.setBounds (8, 5, 120, 22);
    notesBtn.setBounds (138, 5, 160, 22);
    plot = { KEYW, TOPBAR, juce::jmax (10, getWidth() - KEYW - AXW), juce::jmax (10, getHeight() - TOPBAR - 6) };
    history = juce::Image (juce::Image::ARGB, plot.getWidth(), plot.getHeight(), true);
    juce::Graphics g (history);
    g.fillAll (juce::Colours::black);
}

float SpectroEditor::yOf (float f) const
{
    return (float) plot.getY() + (float) plot.getHeight() * (1.0f - std::log (f / FMIN) / LOGR);
}

void SpectroEditor::addColumns()
{
    const int need = proc.resBands.load() * 8;
    if (need != fftSize) setFftSize (need);

    const int w = proc.writePos.load (std::memory_order_acquire);
    const int mask = SpectroProcessor::ringSize - 1;
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = proc.ring[(size_t) ((w - fftSize + i) & mask)] * window[(size_t) i];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    fft->performFrequencyOnlyForwardTransform (fftData.data());

    const int bins = fftSize / 2;
    const double nyq = proc.sampleRateAtomic.load() * 0.5;
    const int N = proc.resBands.load();
    const float norm = (float) fftSize * 0.25f;

    bands.resize ((size_t) N);
    for (int i = 0; i < N; ++i)
    {
        const double p0 = (double) FMIN * std::exp ((double) i / N * LOGR) / nyq * bins;
        const double p1 = (double) FMIN * std::exp ((double) (i + 1) / N * LOGR) / nyq * bins;
        float m;
        if (p1 - p0 < 1.0)
        {
            const double p = 0.5 * (p0 + p1);
            const int k = juce::jlimit (0, bins - 2, (int) p);
            const float t = (float) (p - k);
            m = fftData[(size_t) k] * (1.0f - t) + fftData[(size_t) k + 1] * t;
        }
        else
        {
            m = 0.0f;
            const int b0 = juce::jmax (1, (int) p0), b1 = juce::jmin (bins - 1, (int) std::ceil (p1));
            for (int b = b0; b <= b1; ++b) m = juce::jmax (m, fftData[(size_t) b]);
        }
        bands[(size_t) i] = 20.0f * std::log10 (m / norm + 1.0e-9f);
    }

    const int rows = history.getHeight(), cols = history.getWidth();
    const int step = 2;
    juce::Image::BitmapData bd (history, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < rows; ++y)
    {
        auto* line = bd.getLinePointer (y);
        std::memmove (line, line + step * bd.pixelStride, (size_t) (cols - step) * (size_t) bd.pixelStride);
    }

    float pv = 0.0f; int py = 0;
    for (int y = 0; y < rows; ++y)
    {
        const int i0 = juce::jmin (N - 1, (int) ((1.0f - (float) (y + 1) / rows) * N));
        const int i1 = juce::jmin (N - 1, juce::jmax (i0, (int) std::ceil ((1.0f - (float) y / rows) * N) - 1));
        float m = -200.0f;
        for (int i = i0; i <= i1; ++i) m = juce::jmax (m, bands[(size_t) i]);
        const float v = juce::jlimit (0.0f, 1.0f, (m - DBMIN) / (DBMAX - DBMIN));
        if (v > pv) { pv = v; py = y; }
        const auto c = lut[(size_t) juce::roundToInt (std::pow (v, 0.8f) * 255.0f)];
        for (int x = 0; x < step; ++x) bd.setPixelColour (cols - step + x, y, c);
    }
    peakV = pv;
    peakF = FMIN * std::exp ((1.0f - ((float) py + 0.5f) / (float) rows) * LOGR);
}

void SpectroEditor::timerCallback()
{
    if (history.isNull()) return;
    addColumns();
    repaint();
}

void SpectroEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c2227));
    g.setColour (juce::Colours::black);
    g.fillRect (plot.withX (0).withWidth (getWidth()));
    if (history.isNull()) return;
    g.drawImageAt (history, plot.getX(), plot.getY());

    const int x0 = plot.getX(), x1 = plot.getRight();
    g.setFont (11.0f);

    // octave lines (every C) and optional lines for the other notes
    for (int n = 24; n <= 132; ++n)
    {
        const float f = noteFreq (n);
        if (f < FMIN || f > FMAX) continue;
        const float y = std::floor (yOf (f)) + 0.5f;
        if (n % 12 == 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.drawHorizontalLine ((int) y, 4.0f, (float) x1);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            const juce::String hz = f < 1000.0f ? juce::String (f, f < 100.0f ? 1 : 0) : juce::String (f / 1000.0f, 2) + "k";
            g.drawText ("C" + juce::String (n / 12 - 1) + "  " + hz, x0 + 6, (int) y - 16, 120, 14, juce::Justification::left);
        }
        else if (showNotes)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawHorizontalLine ((int) y, (float) x0, (float) x1);
        }
    }

    // piano strip
    for (int n = 24; n <= 132; ++n)
    {
        const float fa = noteFreq (n) / std::pow (2.0f, 0.5f / 12.0f), fb = noteFreq (n) * std::pow (2.0f, 0.5f / 12.0f);
        if (fb < FMIN || fa > FMAX) continue;
        const float ya = yOf (juce::jmin (fb, FMAX)), yb = yOf (juce::jmax (fa, FMIN));
        const int pc = n % 12;
        const bool blk = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        g.setColour (blk ? juce::Colour (0xff2a2f33) : (pc == 0 ? juce::Colours::white : juce::Colour (0xffaeb5b9)));
        g.fillRect (4.0f, ya + 0.5f, blk ? (float) KEYW - 18.0f : (float) KEYW - 10.0f, juce::jmax (yb - ya - 1.0f, 0.8f));
    }

    // Hz axis
    for (float f : { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 15000.f, 20000.f })
    {
        const float y = juce::jlimit ((float) plot.getY() + 6.0f, (float) plot.getBottom() - 6.0f, yOf (f));
        g.setColour (juce::Colour (0xff8b969c));
        g.drawText (f >= 1000.0f ? juce::String (f / 1000.0f) + "kHz" : juce::String ((int) f) + "Hz",
                    x1 + 6, (int) y - 7, AXW - 6, 14, juce::Justification::left);
    }

    // current pitch line and readout
    if (peakV > 0.3f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawHorizontalLine ((int) yOf (peakF), (float) x0, (float) x1);
        g.setColour (juce::Colour (0xffe6ebed));
        g.setFont (12.0f);
        g.drawText (noteName (peakF) + "  " + juce::String ((int) std::lround (peakF)) + "Hz",
                    x1 - 160, plot.getBottom() - 20, 154, 16, juce::Justification::right);
    }
}
