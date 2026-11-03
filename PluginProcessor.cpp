#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    using NR  = juce::NormalisableRange<float>;

    NR skewed (float lo, float hi, float centre)
    {
        NR r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    std::unique_ptr<APF> fp (const char* id, const char* name, NR range, float def, const char* unit, int decimals = 0)
    {
        return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def,
                                      juce::AudioParameterFloatAttributes().withLabel (unit)
                                          .withStringFromValueFunction ([decimals, unit] (float v, int)
                                          { return juce::String (v, decimals) + (juce::String (unit).isEmpty() ? "" : " " + juce::String (unit)); }));
    }

    //==========================================================================
    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list {
            { "Trap Crack",    {} },
            { "Atlanta Clap",  { { "bodyLevel", 35 }, { "noiseLevel", 50 }, { "clapLevel", 90 }, { "clapSpread", 11 },
                                 { "clapTone", 1300 }, { "clapDecay", 180 }, { "reverb", 20 }, { "snap", 35 } } },
            { "Tight Rim",     { { "tune", 330 }, { "drop", 8 }, { "bodyDecay", 50 }, { "noiseDecay", 70 }, { "snap", 85 },
                                 { "clapLevel", 0 }, { "reverb", 5 }, { "drive", 9 }, { "noiseTone", 4500 } } },
            { "Big Room",      { { "reverb", 45 }, { "room", 85 }, { "noiseDecay", 260 }, { "bodyDecay", 160 },
                                 { "clapLevel", 55 }, { "clapDecay", 220 } } },
            { "Pitched Down",  { { "tune", 150 }, { "drop", 20 }, { "dropTime", 40 }, { "bodyDecay", 230 }, { "bodyLevel", 85 },
                                 { "noiseTone", 2000 }, { "drive", 12 }, { "clapLevel", 25 } } },
            { "Lo-Fi Crunch",  { { "level", 2 }, { "drive", 22 }, { "noiseTone", 1500 }, { "reverb", 10 }, { "snap", 70 }, { "clapLevel", 30 } } },
            { "Roll Ready",    { { "roll", 1 }, { "rollRate", 4 }, { "rollRamp", 50 }, { "rollPitch", 0.5f }, { "bodyDecay", 80 },
                                 { "noiseDecay", 110 }, { "clapLevel", 30 }, { "reverb", 12 } } },
        };
        return list;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TrapSnareProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (fp ("tune",       "Tune",        skewed (100.0f, 500.0f, 220.0f),   200.0f, "Hz"));
    l.add (fp ("drop",       "Pitch Drop",  NR (0.0f, 36.0f),                  14.0f,  "st", 1));
    l.add (fp ("dropTime",   "Drop Time",   skewed (3.0f, 200.0f, 30.0f),      25.0f,  "ms"));
    l.add (fp ("bodyDecay",  "Body Decay",  skewed (20.0f, 800.0f, 150.0f),    110.0f, "ms"));
    l.add (fp ("bodyLevel",  "Body Level",  NR (0.0f, 100.0f),                 70.0f,  "%"));

    l.add (fp ("noiseTone",  "Noise Tone",  skewed (400.0f, 12000.0f, 3000.0f), 3000.0f, "Hz"));
    l.add (fp ("noiseDecay", "Noise Decay", skewed (20.0f, 1000.0f, 180.0f),   160.0f, "ms"));
    l.add (fp ("snap",       "Snap",        NR (0.0f, 100.0f),                 50.0f,  "%"));
    l.add (fp ("noiseLevel", "Noise Level", NR (0.0f, 100.0f),                 75.0f,  "%"));

    l.add (fp ("clapLevel",  "Clap Level",  NR (0.0f, 100.0f),                 40.0f,  "%"));
    l.add (fp ("clapTone",   "Clap Tone",   skewed (500.0f, 5000.0f, 1500.0f), 1500.0f, "Hz"));
    l.add (fp ("clapSpread", "Clap Spread", NR (3.0f, 25.0f),                  10.0f,  "ms", 1));
    l.add (fp ("clapDecay",  "Clap Decay",  skewed (20.0f, 800.0f, 150.0f),    140.0f, "ms"));

    l.add (fp ("drive",      "Drive",       NR (0.0f, 30.0f),                  6.0f,   "dB", 1));
    l.add (fp ("reverb",     "Reverb",      NR (0.0f, 100.0f),                 15.0f,  "%"));
    l.add (fp ("room",       "Room Size",   NR (0.0f, 100.0f),                 50.0f,  "%"));

    l.add (fp ("variation",  "Variation",   NR (0.0f, 100.0f),                 15.0f,  "%"));
    l.add (fp ("velSens",    "Velocity",    NR (0.0f, 100.0f),                 70.0f,  "%"));
    l.add (fp ("level",      "Level",       NR (-30.0f, 6.0f),                 -3.0f,  "dB", 1));
    l.add (std::make_unique<APB> (juce::ParameterID { "keyTrack", 1 }, "Key Track", false));

    l.add (std::make_unique<APB> (juce::ParameterID { "roll", 1 }, "Roll Mode", false));
    l.add (std::make_unique<APC> (juce::ParameterID { "rollRate", 1 }, "Roll Rate", ts::rollRateNames(), 4));
    l.add (fp ("rollRamp",   "Roll Ramp",   NR (-100.0f, 100.0f),              40.0f,  "%"));
    l.add (fp ("rollPitch",  "Roll Pitch",  NR (-2.0f, 2.0f, 0.05f),           0.0f,   "st", 2));

    return l;
}

TrapSnareProcessor::TrapSnareProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool TrapSnareProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

ts::Params TrapSnareProcessor::readParams() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    ts::Params p;
    p.tuneHz       = v ("tune");
    p.dropSemis    = v ("drop");
    p.dropMs       = v ("dropTime");
    p.bodyDecayMs  = v ("bodyDecay");
    p.bodyLevel    = v ("bodyLevel") / 100.0f;
    p.noiseToneHz  = v ("noiseTone");
    p.noiseDecayMs = v ("noiseDecay");
    p.snap         = v ("snap") / 100.0f;
    p.noiseLevel   = v ("noiseLevel") / 100.0f;
    p.clapLevel    = v ("clapLevel") / 100.0f;
    p.clapToneHz   = v ("clapTone");
    p.clapSpreadMs = v ("clapSpread");
    p.clapDecayMs  = v ("clapDecay");
    p.driveDb      = v ("drive");
    p.reverbMix    = v ("reverb") / 100.0f;
    p.roomSize     = v ("room") / 100.0f;
    p.variation    = v ("variation") / 100.0f;
    p.velSens      = v ("velSens") / 100.0f;
    p.levelDb      = v ("level");
    p.keyTrack     = v ("keyTrack");
    p.roll         = v ("roll");
    p.rollRate     = v ("rollRate");
    p.rollRamp     = v ("rollRamp") / 100.0f;
    p.rollPitch    = v ("rollPitch");
    return p;
}

void TrapSnareProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    engine.setParams (readParams());
}

void TrapSnareProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    engine.setParams (readParams());

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                engine.setTempo (*bpm);

    if (auditionRequested.exchange (false))
        engine.hit (60, 0.9f);

    engine.process (buffer, midi);
}

//==============================================================================
int TrapSnareProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String TrapSnareProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? presets()[(size_t) index].name : "";
}

void TrapSnareProcessor::setParam (const juce::String& id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
}

void TrapSnareProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms())) return;
    currentPreset = index;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& [id, value] : presets()[(size_t) index].values)
        setParam (id, value);
}

void TrapSnareProcessor::randomize()
{
    juce::Random r;
    auto pick = [&r] (float lo, float hi) { return lo + r.nextFloat() * (hi - lo); };

    setParam ("tune",       pick (150, 320));
    setParam ("drop",       pick (6, 24));
    setParam ("dropTime",   pick (10, 60));
    setParam ("bodyDecay",  pick (50, 220));
    setParam ("bodyLevel",  pick (35, 90));
    setParam ("noiseTone",  pick (1500, 6000));
    setParam ("noiseDecay", pick (70, 260));
    setParam ("snap",       pick (20, 90));
    setParam ("noiseLevel", pick (45, 90));
    setParam ("clapLevel",  r.nextFloat() < 0.3f ? 0.0f : pick (20, 90));
    setParam ("clapTone",   pick (900, 2500));
    setParam ("clapSpread", pick (6, 14));
    setParam ("clapDecay",  pick (80, 240));
    setParam ("drive",      pick (0, 16));
    setParam ("reverb",     pick (5, 35));
    setParam ("room",       pick (20, 80));
}

//==============================================================================
void TrapSnareProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void TrapSnareProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            // make sure the host sees every restored value, even ones that didn't change internally
            for (auto* param : getParameters())
                param->sendValueChangedMessageToListeners (param->getValue());
        }
}

juce::AudioProcessorEditor* TrapSnareProcessor::createEditor()
{
    return new TrapSnareEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TrapSnareProcessor();
}
