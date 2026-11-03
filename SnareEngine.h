#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>

namespace ts
{

//==============================================================================
/** All sound settings. Everything is a float so the struct can be compared with memcmp. */
struct Params
{
    // Body (pitched tone)
    float tuneHz       = 200.0f;
    float dropSemis    = 14.0f;
    float dropMs       = 25.0f;
    float bodyDecayMs  = 110.0f;
    float bodyLevel    = 0.70f;

    // Noise (rattle) + transient snap
    float noiseToneHz  = 3000.0f;
    float noiseDecayMs = 160.0f;
    float snap         = 0.50f;
    float noiseLevel   = 0.75f;

    // Clap
    float clapLevel    = 0.40f;
    float clapToneHz   = 1500.0f;
    float clapSpreadMs = 10.0f;
    float clapDecayMs  = 140.0f;

    // FX
    float driveDb      = 6.0f;
    float reverbMix    = 0.15f;
    float roomSize     = 0.50f;

    // Global
    float variation    = 0.15f;
    float velSens      = 0.70f;
    float levelDb      = -3.0f;
    float keyTrack     = 0.0f;   // 0/1

    // Rolls
    float roll         = 0.0f;   // 0/1
    float rollRate     = 4.0f;   // index into rollRateBeats
    float rollRamp     = 0.40f;  // -1..1 velocity change across the roll
    float rollPitch    = 0.0f;   // semitones per hit

    bool operator== (const Params& o) const noexcept { return std::memcmp (this, &o, sizeof (Params)) == 0; }
    bool operator!= (const Params& o) const noexcept { return ! operator== (o); }
};

inline const juce::StringArray& rollRateNames()
{
    static const juce::StringArray names { "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T", "1/64" };
    return names;
}

inline double rollRateBeats (int index)
{
    static const double beats[] { 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125, 1.0 / 12.0, 0.0625 };
    return beats[juce::jlimit (0, 6, index)];
}

//==============================================================================
struct Rng
{
    uint32_t s = 0x9e3779b9u;
    void seed (uint32_t v) noexcept { s = v ? v : 0x9e3779b9u; }
    inline uint32_t next() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float white() noexcept { return (float) (next() >> 8) * (2.0f / 16777216.0f) - 1.0f; }
    inline float uni()   noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float gauss() noexcept
    {
        const float u = juce::jmax (1.0e-7f, uni()), v = uni();
        return std::sqrt (-2.0f * std::log (u)) * std::cos (juce::MathConstants<float>::twoPi * v);
    }
};

/** Topology-preserving state-variable filter (Cytomic / Simper). */
struct Svf
{
    float a1 = 0, a2 = 0, a3 = 0, k = 1, ic1 = 0, ic2 = 0;

    void set (float fc, float q, float sr) noexcept
    {
        fc = juce::jlimit (20.0f, sr * 0.45f, fc);
        const float g = std::tan (juce::MathConstants<float>::pi * fc / sr);
        k  = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() noexcept { ic1 = ic2 = 0; }

    inline void process (float v0, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
};

//==============================================================================
/** One snare hit. */
struct Voice
{
    bool     active = false;
    double   t = 0.0;          // seconds since the hit
    float    gain = 1.0f, pitchRatio = 1.0f;
    float    varPitch = 1.0f, varDecay = 1.0f, varSpread = 1.0f;
    double   phase1 = 0.0, phase2 = 0.0;
    double   endTime = 1.0;
    uint64_t order = 0;
    Svf noiseHp, noiseBp, clickHp, clapBp;
    Rng rng;

    void start (float newGain, float newPitchRatio, float variation, Rng& seedSource, uint64_t ord)
    {
        active = true;
        t = 0.0;
        phase1 = phase2 = 0.0;
        order = ord;
        rng.seed (seedSource.next());
        noiseHp.reset(); noiseBp.reset(); clickHp.reset(); clapBp.reset();

        varPitch  = std::exp2 (variation * 0.35f * juce::jlimit (-2.5f, 2.5f, seedSource.gauss()) / 12.0f);
        varDecay  = 1.0f + variation * 0.18f * (seedSource.uni() * 2.0f - 1.0f);
        varSpread = 1.0f + variation * 0.25f * (seedSource.uni() * 2.0f - 1.0f);
        gain      = newGain * juce::Decibels::decibelsToGain (variation * 1.5f * (seedSource.uni() * 2.0f - 1.0f));
        pitchRatio = newPitchRatio;
    }

    void updateFilters (const Params& p, float sr)
    {
        noiseHp.set (p.noiseToneHz, 0.707f, sr);
        noiseBp.set (p.noiseToneHz * 1.6f, 1.2f, sr);
        clickHp.set (1500.0f, 0.707f, sr);
        clapBp.set (p.clapToneHz, 2.2f, sr);

        const double longest = juce::jmax ((double) p.bodyDecayMs, (double) p.noiseDecayMs, (double) p.clapDecayMs) / 1000.0;
        endTime = longest * 1.25 * 7.0 + 4.0 * p.clapSpreadMs / 1000.0 + 0.01;
    }

    void render (const Params& p, float sr, float* out, int num)
    {
        const float dt = 1.0f / sr;
        const float dropTau   = juce::jmax (0.001f, p.dropMs / 1000.0f);
        const float bodyTau   = juce::jmax (0.002f, p.bodyDecayMs / 1000.0f * varDecay);
        const float noiseTau  = juce::jmax (0.002f, p.noiseDecayMs / 1000.0f * varDecay);
        const float clapTau   = juce::jmax (0.002f, p.clapDecayMs / 1000.0f * varDecay);
        const float spread    = juce::jmax (0.001f, p.clapSpreadMs / 1000.0f * varSpread);
        const float baseFreq  = p.tuneHz * pitchRatio * varPitch;
        const float twoPi     = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < num; ++i)
        {
            const float tt = (float) t;

            // --- body: two membrane modes with a fast pitch drop
            const float semis = p.dropSemis * std::exp (-tt / dropTau);
            const float f = baseFreq * std::exp2 (semis / 12.0f);
            phase1 += f * dt;          phase1 -= std::floor (phase1);
            phase2 += f * 1.59f * dt;  phase2 -= std::floor (phase2);
            const float attack = juce::jmin (1.0f, tt / 0.0004f);
            const float bodyEnv = attack * std::exp (-tt / bodyTau);
            const float body = (std::sin (twoPi * (float) phase1)
                                + 0.35f * std::sin (twoPi * (float) phase2) * std::exp (-tt / (bodyTau * 0.5f))) * bodyEnv;

            // --- noise rattle + snap transient
            float lp, bp, hp, bp2, hp2, dummy;
            const float n = rng.white();
            noiseHp.process (n, lp, bp, hp);
            noiseBp.process (n, dummy, bp2, hp2);
            const float noise = (0.8f * hp + 0.7f * bp2) * std::exp (-tt / noiseTau);

            float cbp, chp;
            clickHp.process (n, dummy, cbp, chp);
            const float click = chp * p.snap * 1.6f * std::exp (-tt / 0.0025f);

            // --- clap: three quick bursts and a tail
            float clapEnv = 0.0f;
            for (int k = 0; k < 3; ++k)
            {
                const float tk = tt - (float) k * spread;
                if (tk >= 0.0f) clapEnv += std::exp (-tk / 0.0028f);
            }
            const float tl = tt - 3.0f * spread;
            if (tl >= 0.0f) clapEnv += std::exp (-tl / clapTau);
            float clp, cbp2, chp2;
            clapBp.process (rng.white(), clp, cbp2, chp2);
            const float clap = cbp2 * clapEnv * 2.2f;

            out[i] += 0.6f * gain * (p.bodyLevel * body + p.noiseLevel * noise + attack * click + p.clapLevel * clap);

            t += dt;
        }

        if (t > endTime)
            active = false;
    }
};

//==============================================================================
class Engine
{
public:
    static constexpr int numVoices = 16;

    void prepare (double newSampleRate, int maxBlock)
    {
        sr = (float) newSampleRate;
        mono.assign ((size_t) juce::jmax (1, maxBlock), 0.0f);
        reverb.setSampleRate (newSampleRate);
        reverb.reset();
        for (auto& v : voices) v.active = false;
        held.clear();
    }

    void setParams (const Params& p)
    {
        params = p;
        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.35f + 0.6f * p.roomSize;
        rp.damping    = 0.45f;
        rp.wetLevel   = p.reverbMix * 0.9f;
        rp.dryLevel   = 1.0f - p.reverbMix * 0.35f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
    }

    uint32_t getHitCount() const noexcept { return hitCount.load(); }

    void setTempo (double bpm) noexcept { tempo = bpm > 20.0 ? bpm : 140.0; }

    /** Trigger one hit right now (used for audition and preview). */
    void hit (int note, float velocity01, int rollIndex = 0)
    {
        trigger (note, velocity01, rollIndex);
    }

    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
    {
        const int n = buffer.getNumSamples();
        if ((int) mono.size() < n) mono.resize ((size_t) n);
        std::fill (mono.begin(), mono.begin() + n, 0.0f);

        for (auto& v : voices)
            if (v.active) v.updateFilters (params, sr);

        const bool rolling = params.roll > 0.5f;
        const int  interval = juce::jmax (8, (int) std::lround (60.0 / tempo * rollRateBeats ((int) params.rollRate) * sr));

        auto it = midi.begin();
        int pos = 0;
        while (pos < n)
        {
            int segEnd = n;
            if (it != midi.end())
                segEnd = juce::jmin (segEnd, juce::jmax (pos, (*it).samplePosition));
            if (rolling)
                for (auto& h : held)
                    segEnd = juce::jmin (segEnd, pos + juce::jmax (0, h.samplesToNext));

            renderVoices (pos, segEnd - pos);
            for (auto& h : held)
                h.samplesToNext = rolling ? h.samplesToNext - (segEnd - pos) : interval;
            pos = segEnd;
            if (pos >= n) break;

            while (it != midi.end() && (*it).samplePosition <= pos)
            {
                handleMidi ((*it).getMessage(), interval);
                ++it;
            }

            if (rolling)
                for (auto& h : held)
                    while (h.samplesToNext <= 0)
                    {
                        ++h.hits;
                        const float ramp = std::exp (params.rollRamp * 0.12f * (float) h.hits);
                        trigger (h.note, juce::jlimit (0.03f, 1.0f, h.velocity * ramp), h.hits);
                        h.samplesToNext += interval;
                    }
        }
        // any MIDI after the last sample (shouldn't happen, but be safe)
        for (; it != midi.end(); ++it)
            handleMidi ((*it).getMessage(), interval);

        // global chain: drive -> level -> reverb (stereo)
        const float g = juce::Decibels::decibelsToGain (params.driveDb);
        const float makeup = 1.0f / std::sqrt (g);
        const float level = juce::Decibels::decibelsToGain (params.levelDb);
        for (int i = 0; i < n; ++i)
        {
            float x = mono[(size_t) i];
            if (params.driveDb > 0.01f)
                x = std::tanh (x * g) * makeup;
            mono[(size_t) i] = x * level;
        }

        const int chans = buffer.getNumChannels();
        for (int c = 0; c < chans; ++c)
            buffer.copyFrom (c, 0, mono.data(), n);
        if (chans >= 2)
            reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        else if (chans == 1)
            reverb.processMono (buffer.getWritePointer (0), n);

        // output safety limiter (instant attack, smooth release), ceiling -0.3 dBFS
        const float ceiling = 0.966f;
        const float release = std::exp (-1.0f / (0.08f * sr));
        for (int i = 0; i < n; ++i)
        {
            float pk = 0.0f;
            for (int c = 0; c < chans; ++c) pk = juce::jmax (pk, std::abs (buffer.getSample (c, i)));
            limEnv = juce::jmax (pk, limEnv * release);
            if (limEnv > ceiling)
            {
                const float limGain = ceiling / limEnv;
                for (int c = 0; c < chans; ++c) buffer.setSample (c, i, buffer.getSample (c, i) * limGain);
            }
        }
    }

    /** Renders one hit with the given settings (no variation), for the editor's waveform. */
    static std::vector<float> renderPreview (Params p, double sampleRate, double seconds)
    {
        p.variation = 0.0f;
        p.roll = 0.0f;
        Engine e;
        const int total = (int) (sampleRate * seconds), block = 512;
        e.prepare (sampleRate, block);
        e.setParams (p);
        e.rng.seed (1234);
        e.hit (60, 1.0f);
        std::vector<float> out;
        out.reserve ((size_t) total);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer none;
        for (int done = 0; done < total; done += block)
        {
            buf.clear();
            e.process (buf, none);
            for (int i = 0; i < block && done + i < total; ++i)
                out.push_back (0.5f * (buf.getSample (0, i) + buf.getSample (1, i)));
        }
        return out;
    }

private:
    struct Held { int note; float velocity; int samplesToNext; int hits; };

    void renderVoices (int start, int num)
    {
        if (num <= 0) return;
        for (auto& v : voices)
            if (v.active)
                v.render (params, sr, mono.data() + start, num);
    }

    void handleMidi (const juce::MidiMessage& m, int interval)
    {
        if (m.isNoteOn())
        {
            const float vel = m.getFloatVelocity();
            trigger (m.getNoteNumber(), vel, 0);
            held.erase (std::remove_if (held.begin(), held.end(), [&] (const Held& h) { return h.note == m.getNoteNumber(); }), held.end());
            if (held.size() < 16)
                held.push_back ({ m.getNoteNumber(), vel, interval, 0 });
        }
        else if (m.isNoteOff())
        {
            held.erase (std::remove_if (held.begin(), held.end(), [&] (const Held& h) { return h.note == m.getNoteNumber(); }), held.end());
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            held.clear();
            if (m.isAllSoundOff())
                for (auto& v : voices) v.active = false;
        }
    }

    void trigger (int note, float velocity01, int rollIndex)
    {
        Voice* target = nullptr;
        for (auto& v : voices)
            if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices)
                if (v.order < target->order) target = &v;
        }

        const float velCurve = std::pow (juce::jlimit (0.0f, 1.0f, velocity01), 1.4f);
        const float gain = (1.0f - params.velSens) + params.velSens * velCurve;

        float semis = juce::jlimit (-36.0f, 36.0f, params.rollPitch * (float) rollIndex);
        if (params.keyTrack > 0.5f)
            semis += (float) (note - 60);

        hitCount.fetch_add (1);
        target->start (gain, std::exp2 (semis / 12.0f), params.variation, rng, ++counter);
        target->updateFilters (params, sr);
    }

    Params   params;
    float    sr = 44100.0f;
    double   tempo = 140.0;
    uint64_t counter = 0;
    std::atomic<uint32_t> hitCount { 0 };
    Rng      rng { (uint32_t) juce::Time::getMillisecondCounter() | 1u };
    std::array<Voice, numVoices> voices;
    std::vector<Held>  held;
    std::vector<float> mono;
    juce::Reverb reverb;
    float limEnv = 0.0f;
};

} // namespace ts
