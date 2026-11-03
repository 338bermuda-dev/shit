// Renders every preset (plus rolls) to a WAV demo, checks the audio, and snapshots the UI.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

static int failures = 0;
static void check (bool ok, const juce::String& msg)
{
    std::cout << (ok ? "ok:   " : "FAIL: ") << msg << "\n";
    if (! ok) ++failures;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : ".");
    const double sr = 44100.0;
    const int block = 256;
    const double beat = 60.0 / 140.0;

    TrapSnareProcessor proc;
    juce::AudioBuffer<float> demo (2, 0);
    int demoLen = 0;

    auto append = [&] (const juce::AudioBuffer<float>& b)
    {
        demo.setSize (2, demoLen + b.getNumSamples(), true, true);
        for (int c = 0; c < 2; ++c) demo.copyFrom (c, demoLen, b, c, 0, b.getNumSamples());
        demoLen += b.getNumSamples();
    };

    // events: (time seconds, note, velocity 0..1; 0 = note-off)
    auto render = [&] (const juce::String& name, std::vector<std::tuple<double, int, float>> events, double seconds,
                       std::function<void()> setup)
    {
        setup();
        proc.prepareToPlay (sr, block);
        const auto hitsBefore = proc.getHitCount();
        const int total = (int) (seconds * sr);
        juce::AudioBuffer<float> out (2, total), buf (2, block);
        bool finite = true;
        for (int pos = 0; pos < total; pos += block)
        {
            juce::MidiBuffer midi;
            for (auto& [t, note, vel] : events)
            {
                const int s = (int) std::lround (t * sr);
                if (s >= pos && s < pos + block)
                    midi.addEvent (vel > 0 ? juce::MidiMessage::noteOn (1, note, vel) : juce::MidiMessage::noteOff (1, note), s - pos);
            }
            proc.processBlock (buf, midi);
            const int n = juce::jmin (block, total - pos);
            for (int c = 0; c < 2; ++c)
            {
                for (int i = 0; i < n; ++i) finite &= std::isfinite (buf.getSample (c, i));
                out.copyFrom (c, pos, buf, c, 0, n);
            }
        }
        const float peak = out.getMagnitude (0, total);
        const float peakDb = juce::Decibels::gainToDecibels (peak, -100.0f);
        const auto hits = proc.getHitCount() - hitsBefore;
        check (finite, name + ": audio is finite");
        check (peakDb > -18.0f && peakDb <= 0.0f, name + ": peak " + juce::String (peakDb, 1) + " dBFS");
        append (out);
        return hits;
    };

    const std::vector<std::tuple<double, int, float>> groove {
        { 0.0, 60, 1.0f }, { 0.05, 60, 0.0f }, { 2 * beat, 60, 0.85f }, { 2 * beat + 0.05, 60, 0.0f } };

    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        const auto name = proc.getProgramName (i);
        if (name == "Roll Ready")
        {
            const auto hits = render (name, { { 0.0, 60, 0.6f }, { 2 * beat, 60, 0.0f } }, 4 * beat,
                                      [&] { proc.setCurrentProgram (i); });
            check (hits >= 15 && hits <= 17, name + ": 1/32 roll over 2 beats = " + juce::String ((int) hits) + " hits");
        }
        else
            render (name, groove, 4 * beat, [&] { proc.setCurrentProgram (i); });
    }

    // a rising triplet roll, then a crescendo 1/16 roll on the default sound
    auto set = [&] (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    render ("Triplet riser", { { 0.0, 60, 0.5f }, { 2 * beat, 60, 0.0f } }, 3 * beat, [&]
    {
        proc.setCurrentProgram (0);
        set ("roll", 1); set ("rollRate", 3); set ("rollPitch", 1.0f); set ("rollRamp", 60);
    });
    const auto hits = render ("1/16 crescendo", { { 0.0, 62, 0.35f }, { 1 * beat, 62, 0.0f } }, 3 * beat, [&]
    {
        proc.setCurrentProgram (0);
        set ("roll", 1); set ("rollRate", 2); set ("rollPitch", 0.0f); set ("rollRamp", 100);
    });
    check (hits == 4, "1/16 roll over 1 beat = " + juce::String ((int) hits) + " hits");

    // key tracking: C4 should be an octave above C3 -> zero-crossing rate roughly doubles
    {
        proc.setCurrentProgram (0);
        set ("keyTrack", 1); set ("noiseLevel", 0); set ("clapLevel", 0); set ("snap", 0); set ("reverb", 0);
        set ("variation", 0); set ("drop", 0); set ("drive", 0);
        auto zc = [&] (int note)
        {
            proc.prepareToPlay (sr, 4096);
            juce::AudioBuffer<float> b (2, 4096);
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, note, 1.0f), 0);
            proc.processBlock (b, m);
            int z = 0;
            for (int i = 1; i < 4096; ++i) z += (b.getSample (0, i - 1) < 0) != (b.getSample (0, i) < 0);
            return z;
        };
        const int lo = zc (60), hi = zc (72);
        check (hi > lo * 1.7 && hi < lo * 2.3, "key track: C4 vs C3 zero crossings " + juce::String (hi) + " vs " + juce::String (lo));
    }

    // write demo
    {
        auto f = outDir.getChildFile ("TrapSnare-demo.wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        auto* stream = new juce::FileOutputStream (f);
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (stream, sr, 2, 24, {}, 0));
        w->writeFromAudioSampleBuffer (demo, 0, demoLen);
        std::cout << "demo: " << f.getFullPathName() << " (" << demoLen / sr << " s)\n";
    }

    // UI snapshot
    {
        proc.setCurrentProgram (0);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
        auto f = outDir.getChildFile ("TrapSnare-ui.png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
    }

    std::cout << (failures ? "\nSOME TESTS FAILED\n" : "\nALL TESTS PASSED\n");
    return failures ? 1 : 0;
}
