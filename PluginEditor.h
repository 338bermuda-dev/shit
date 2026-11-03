#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
/** Diamonds set in gold. Everything bussed down. */
namespace ice
{
    /** Pre-rendered brilliant-cut stones (a few variants so pavé looks alive). */
    const juce::Image& stone (int variant);

    void drawStone (juce::Graphics&, juce::Point<float> centre, float diameter, int variant);
    /** Fill an arbitrary shape with pavé diamonds over a gold bed. */
    void pave (juce::Graphics&, const juce::Path& shape, float stoneSize, uint32_t seed = 1);
    /** A line of stones following a path (tennis-chain style). */
    void stoneLine (juce::Graphics&, const juce::Path& path, float stoneSize, float spacing, uint32_t seed = 1);
    void drawGem (juce::Graphics&, juce::Rectangle<float> area, float glow);
    void drawSparkle (juce::Graphics&, juce::Point<float> c, float radius, float alpha);
    juce::ColourGradient goldGradient (juce::Rectangle<float> r, bool vertical = true);
}

class BlingLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BlingLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

//==============================================================================
class WaveformView : public juce::Component
{
public:
    void setData (std::vector<float> d);
    void setFlash (float f);
    void paint (juce::Graphics&) override;
    void resized() override { rebuild(); }

private:
    void rebuild();

    std::vector<float> data;
    juce::Image cache;
    float peak = 0.0f, flash = 0.0f;
};

//==============================================================================
class TrapSnareEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TrapSnareEditor (TrapSnareProcessor&);
    ~TrapSnareEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderBackground();

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct Section
    {
        juce::String title;
        std::vector<Knob*> knobs;
        std::vector<juce::Component*> extras;
        juce::Rectangle<int> bounds;
    };
    struct Sparkle { juce::Point<float> pos; float phase, speed, size; };

    Knob& addKnob (const juce::String& paramId, const juce::String& text);

    TrapSnareProcessor& proc;
    BlingLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;

    juce::ToggleButton rollToggle { "ROLL" }, keyTrackToggle { "KEY TRACK" };
    juce::ComboBox     rollRate, presetBox;
    juce::TextButton   randomButton { "RANDOMIZE" }, hitButton { "HIT IT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rollAttachment, keyTrackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rollRateAttachment;

    WaveformView waveform;
    juce::Image background;
    juce::Path titlePath;
    juce::Rectangle<int> titleArea;
    std::vector<Sparkle> sparkles;

    ts::Params lastParams;
    bool firstUpdate = true;
    uint32_t lastHits = 0;
    float flash = 0.0f, shimmer = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrapSnareEditor)
};
