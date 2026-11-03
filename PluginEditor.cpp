#include "PluginEditor.h"

namespace bling
{
    const juce::Colour black      { 0xff07070a };
    const juce::Colour panel      { 0xff111015 };
    const juce::Colour goldLight  { 0xfffff2b0 };
    const juce::Colour gold       { 0xffe8b93a };
    const juce::Colour goldMid    { 0xffc8901c };
    const juce::Colour goldDark   { 0xff7a5208 };
    const juce::Colour chrome     { 0xffd9dde6 };
    const juce::Colour chromeDim  { 0xff8c919c };
    const juce::Colour iceBlue    { 0xffbfeaff };
}

using namespace bling;

//==============================================================================
namespace ice
{
    juce::ColourGradient goldGradient (juce::Rectangle<float> r, bool vertical)
    {
        juce::ColourGradient cg (goldLight, r.getX(), r.getY(),
                                 goldDark, vertical ? r.getX() : r.getRight(), vertical ? r.getBottom() : r.getY(), false);
        cg.addColour (0.35, gold);
        cg.addColour (0.55, goldMid);
        cg.addColour (0.75, goldLight.interpolatedWith (gold, 0.5f));
        return cg;
    }

    static juce::Image makeStone (int variant)
    {
        const int S = 72;
        juce::Image img (juce::Image::ARGB, S, S, true);
        juce::Graphics g (img);
        const float c = S * 0.5f;
        const float rot = 0.37f + (float) variant * 0.71f;
        const float pi = juce::MathConstants<float>::pi;

        // gold setting
        g.setGradientFill (juce::ColourGradient (goldLight, 0, 0, goldDark, (float) S, (float) S, false));
        g.fillEllipse (1.0f, 1.0f, S - 2.0f, S - 2.0f);

        const float r = c - 5.0f;
        auto stoneArea = juce::Rectangle<float> (r * 2, r * 2).withCentre ({ c, c });

        // body of the stone
        juce::ColourGradient base (juce::Colours::white, c - r * 0.35f * std::cos (rot), c - r * 0.35f * std::sin (rot),
                                   juce::Colour (0xff6e93a8), c + r, c + r * 0.4f, true);
        base.addColour (0.45, juce::Colour (0xffe4f6ff));
        base.addColour (0.8,  juce::Colour (0xffa9cfe2));
        g.setGradientFill (base);
        g.fillEllipse (stoneArea);

        // kite facets (alternating light / shadow)
        for (int k = 0; k < 8; ++k)
        {
            const float a0 = rot + (float) k * pi / 4.0f, a1 = a0 + pi / 4.0f;
            juce::Path facet;
            facet.startNewSubPath (c + r * 0.42f * std::cos (a0 + pi / 8), c + r * 0.42f * std::sin (a0 + pi / 8));
            facet.lineTo (c + r * std::cos (a0), c + r * std::sin (a0));
            facet.lineTo (c + r * std::cos (a1), c + r * std::sin (a1));
            facet.closeSubPath();
            const bool lit = ((k + variant) % 3) == 0;
            g.setColour (lit ? juce::Colours::white.withAlpha (0.55f) : juce::Colour (0xff4f7d99).withAlpha ((k % 2) ? 0.28f : 0.12f));
            g.fillPath (facet);
        }

        // table (octagon) and facet lines
        juce::Path table;
        for (int k = 0; k < 8; ++k)
        {
            const float a = rot + pi / 8 + (float) k * pi / 4.0f;
            const juce::Point<float> p (c + r * 0.42f * std::cos (a), c + r * 0.42f * std::sin (a));
            if (k == 0) table.startNewSubPath (p); else table.lineTo (p);
        }
        table.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillPath (table);
        g.setColour (juce::Colour (0xff3e6a85).withAlpha (0.55f));
        g.strokePath (table, juce::PathStrokeType (1.2f));
        for (int k = 0; k < 8; ++k)
        {
            const float a = rot + (float) k * pi / 4.0f;
            g.drawLine (c + r * 0.42f * std::cos (a + pi / 8), c + r * 0.42f * std::sin (a + pi / 8),
                        c + r * std::cos (a), c + r * std::sin (a), 1.0f);
        }

        g.setColour (juce::Colour (0xff2e4a5c).withAlpha (0.8f));
        g.drawEllipse (stoneArea, 1.4f);

        // fire: highlight + a little spectral glint
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.fillEllipse (c - r * 0.55f, c - r * 0.62f, r * 0.42f, r * 0.26f);
        const juce::Colour fire[] { juce::Colour (0xffffd6f2), juce::Colour (0xffd2ffe4), juce::Colour (0xfffff3c4) };
        g.setColour (fire[variant % 3].withAlpha (0.7f));
        g.fillEllipse (c + r * 0.2f, c + r * 0.25f, r * 0.22f, r * 0.14f);

        // prongs
        for (int k = 0; k < 4; ++k)
        {
            const float a = pi / 4 + (float) k * pi / 2;
            const juce::Point<float> pp (c + (r + 1.5f) * std::cos (a), c + (r + 1.5f) * std::sin (a));
            g.setGradientFill (juce::ColourGradient (goldLight, pp.x - 3, pp.y - 3, goldMid, pp.x + 3, pp.y + 3, false));
            g.fillEllipse (pp.x - 3.6f, pp.y - 3.6f, 7.2f, 7.2f);
        }
        return img;
    }

    const juce::Image& stone (int variant)
    {
        static const juce::Image stones[] { makeStone (0), makeStone (1), makeStone (2), makeStone (3) };
        return stones[(size_t) (variant & 3)];
    }

    void drawStone (juce::Graphics& g, juce::Point<float> c, float d, int variant)
    {
        g.drawImage (stone (variant), juce::Rectangle<float> (d, d).withCentre (c), juce::RectanglePlacement::stretchToFit);
    }

    static int hashVariant (uint32_t a, uint32_t b, uint32_t seed)
    {
        uint32_t h = a * 73856093u ^ b * 19349663u ^ seed * 83492791u;
        h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        return (int) (h & 3u);
    }

    void pave (juce::Graphics& g, const juce::Path& shape, float s, uint32_t seed)
    {
        const auto b = shape.getBounds().expanded (s);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (shape);
        g.setGradientFill (juce::ColourGradient (goldMid, b.getX(), b.getY(), goldDark, b.getRight(), b.getBottom(), false));
        g.fillRect (b);
        const float dy = s * 0.87f;
        int row = 0;
        for (float y = b.getY(); y < b.getBottom(); y += dy, ++row)
        {
            const float off = (row % 2) ? s * 0.5f : 0.0f;
            int col = 0;
            for (float x = b.getX() + off; x < b.getRight(); x += s, ++col)
                drawStone (g, { x, y }, s * 1.02f, hashVariant ((uint32_t) row, (uint32_t) col, seed));
        }
    }

    void stoneLine (juce::Graphics& g, const juce::Path& path, float s, float spacing, uint32_t seed)
    {
        const float len = path.getLength();
        int i = 0;
        for (float d = spacing * 0.5f; d < len; d += spacing, ++i)
            drawStone (g, path.getPointAlongPath (d), s, hashVariant ((uint32_t) i, 7u, seed));
    }

    void drawSparkle (juce::Graphics& g, juce::Point<float> c, float r, float alpha)
    {
        if (alpha <= 0.01f || r <= 0.5f) return;
        juce::ColourGradient glow (iceBlue.withAlpha (0.5f * alpha), c.x, c.y, iceBlue.withAlpha (0.0f), c.x + r * 1.4f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - r * 1.4f, c.y - r * 1.4f, r * 2.8f, r * 2.8f);

        juce::Path star;
        const float in = r * 0.15f;
        star.startNewSubPath (c.x, c.y - r);
        star.lineTo (c.x + in, c.y - in); star.lineTo (c.x + r, c.y);
        star.lineTo (c.x + in, c.y + in); star.lineTo (c.x, c.y + r);
        star.lineTo (c.x - in, c.y + in); star.lineTo (c.x - r, c.y);
        star.lineTo (c.x - in, c.y - in); star.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillPath (star);
        star.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi / 4, c.x, c.y)
                                 .followedBy (juce::AffineTransform::scale (0.45f, 0.45f, c.x, c.y)));
        g.setColour (juce::Colours::white.withAlpha (alpha * 0.8f));
        g.fillPath (star);
    }

    void drawGem (juce::Graphics& g, juce::Rectangle<float> a, float glow)
    {
        const float w = a.getWidth(), h = a.getHeight(), x = a.getX(), y = a.getY();
        auto P = [&] (float fx, float fy) { return juce::Point<float> (x + fx * w, y + fy * h); };

        if (glow > 0.0f)
        {
            const auto c = a.getCentre();
            juce::ColourGradient halo (iceBlue.withAlpha (0.55f * glow), c.x, c.y, iceBlue.withAlpha (0.0f), c.x + w, c.y, true);
            g.setGradientFill (halo);
            g.fillEllipse (a.expanded (w * 0.5f));
        }

        const auto tl = P (0.25f, 0.0f), tr = P (0.75f, 0.0f), gl = P (0.0f, 0.33f), gr = P (1.0f, 0.33f),
                   cu = P (0.5f, 1.0f), tm = P (0.5f, 0.33f);

        juce::Path crown;  crown.startNewSubPath (tl); crown.lineTo (tr); crown.lineTo (gr); crown.lineTo (gl); crown.closeSubPath();
        juce::Path pavL;   pavL.startNewSubPath (gl);  pavL.lineTo (tm);  pavL.lineTo (cu);  pavL.closeSubPath();
        juce::Path pavR;   pavR.startNewSubPath (tm);  pavR.lineTo (gr);  pavR.lineTo (cu);  pavR.closeSubPath();

        g.setGradientFill (juce::ColourGradient (juce::Colours::white, tl.x, tl.y, iceBlue, gr.x, gr.y, false));
        g.fillPath (crown);
        g.setGradientFill (juce::ColourGradient (iceBlue, gl.x, gl.y, juce::Colour (0xff6fb8e8), cu.x, cu.y, false));
        g.fillPath (pavL);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, tm.x, tm.y, juce::Colour (0xff9fd4f5), cu.x, cu.y, false));
        g.fillPath (pavR);

        g.setColour (juce::Colour (0xff3d7fae).withAlpha (0.7f));
        const float lw = juce::jmax (0.6f, w * 0.03f);
        g.drawLine ({ gl, gr }, lw);
        g.drawLine ({ tl, P (0.38f, 0.33f) }, lw);  g.drawLine ({ tr, P (0.62f, 0.33f) }, lw);
        g.drawLine ({ P (0.38f, 0.33f), cu }, lw);  g.drawLine ({ P (0.62f, 0.33f), cu }, lw);
        juce::Path outline; outline.startNewSubPath (tl); outline.lineTo (tr); outline.lineTo (gr); outline.lineTo (cu);
        outline.lineTo (gl); outline.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.strokePath (outline, juce::PathStrokeType (lw));
    }
}

//==============================================================================
BlingLookAndFeel::BlingLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,        gold);
    setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,                chrome);
    setColour (juce::ComboBox::textColourId,             gold);
    setColour (juce::ComboBox::arrowColourId,            gold);
    setColour (juce::PopupMenu::backgroundColourId,      panel);
    setColour (juce::PopupMenu::textColourId,            gold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, goldMid);
    setColour (juce::PopupMenu::highlightedTextColourId, black);
    setColour (juce::TextButton::textColourOffId,        black);
    setColour (juce::TextButton::textColourOnId,         black);
}

void BlingLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
    const float R = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    auto circle = [&] (float rad) { return juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c); };

    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillEllipse (circle (R * 0.86f).translated (0, R * 0.08f));

    // value arc
    const float arcR = R * 0.94f;
    juce::Path track; track.addCentredArc (c.x, c.y, arcR, arcR, 0, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2418));
    g.strokePath (track, juce::PathStrokeType (R * 0.09f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (pos > 0.001f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, arcR, arcR, 0, startAngle, angle, true);
        g.setColour (iceBlue.withAlpha (0.22f));
        g.strokePath (val, juce::PathStrokeType (R * 0.24f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (goldLight);
        g.strokePath (val, juce::PathStrokeType (R * 0.08f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        ice::drawSparkle (g, { c.x + arcR * std::sin (angle), c.y - arcR * std::cos (angle) }, R * 0.16f, 0.9f);
    }

    // gold mount
    g.setGradientFill (juce::ColourGradient (goldLight, c.x - R, c.y - R, goldDark, c.x + R, c.y + R, false));
    g.fillEllipse (circle (R * 0.84f));

    // two rings of iced-out bezel stones
    const float stoneS = R * 0.17f;
    for (auto [ringR, seed] : { std::pair<float, int> { R * 0.735f, 1 }, { R * 0.585f, 5 } })
    {
        const int n = juce::jmax (8, (int) (juce::MathConstants<float>::twoPi * ringR / (stoneS * 1.02f)));
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) n + (float) seed * 0.1f;
            ice::drawStone (g, { c.x + ringR * std::sin (a), c.y - ringR * std::cos (a) }, stoneS, (i * 7 + seed) & 3);
        }
    }

    // pavé cap
    const float capR = R * 0.49f;
    juce::Path cap; cap.addEllipse (circle (capR));
    ice::pave (g, cap, R * 0.15f, (uint32_t) (R * 10));
    g.setGradientFill (juce::ColourGradient (goldLight, c.x - capR, c.y - capR, goldDark, c.x + capR, c.y + capR, false));
    g.drawEllipse (circle (capR), juce::jmax (1.5f, R * 0.05f));
    g.setGradientFill (juce::ColourGradient (goldLight, c.x - R, c.y - R, goldDark, c.x + R, c.y + R, false));
    g.drawEllipse (circle (R * 0.84f), juce::jmax (1.2f, R * 0.04f));

    // onyx pointer edged in gold, so it reads against all that ice
    const juce::Point<float> tip  (c.x + R * 0.8f * std::sin (angle), c.y - R * 0.8f * std::cos (angle));
    const juce::Point<float> tail (c.x + R * 0.12f * std::sin (angle), c.y - R * 0.12f * std::cos (angle));
    g.setColour (goldLight);
    g.drawLine ({ tail, tip }, R * 0.2f);
    g.setColour (juce::Colour (0xff0d0b10));
    g.drawLine ({ tail, tip }, R * 0.11f);
    g.setColour (goldLight);
    g.fillEllipse (juce::Rectangle<float> (R * 0.16f, R * 0.16f).withCentre (tip));
}

void BlingLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto gemArea = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.16f);
    if (b.getToggleState())
        ice::drawGem (g, gemArea, 1.0f);
    else
    {
        juce::Path p;
        p.startNewSubPath (gemArea.getX() + gemArea.getWidth() * 0.25f, gemArea.getY());
        p.lineTo (gemArea.getRight() - gemArea.getWidth() * 0.25f, gemArea.getY());
        p.lineTo (gemArea.getRight(), gemArea.getY() + gemArea.getHeight() * 0.33f);
        p.lineTo (gemArea.getCentreX(), gemArea.getBottom());
        p.lineTo (gemArea.getX(), gemArea.getY() + gemArea.getHeight() * 0.33f);
        p.closeSubPath();
        g.setColour (chromeDim.withAlpha (highlighted ? 0.8f : 0.5f));
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }
    g.setColour (b.getToggleState() ? goldLight : chromeDim);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (6.0f), juce::Justification::centredLeft);
}

void BlingLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0, 2), 7.0f);

    g.setGradientFill (ice::goldGradient (r));
    g.fillRoundedRectangle (r, 7.0f);
    if (highlighted || down)
    {
        g.setColour (juce::Colours::white.withAlpha (down ? 0.35f : 0.18f));
        g.fillRoundedRectangle (r, 7.0f);
    }

    // diamond rows along top and bottom
    const float s = juce::jmin (7.0f, r.getHeight() * 0.2f);
    juce::Path top, bottom;
    top.startNewSubPath (r.getX() + 8, r.getY() + s * 0.75f);         top.lineTo (r.getRight() - 8, r.getY() + s * 0.75f);
    bottom.startNewSubPath (r.getX() + 8, r.getBottom() - s * 0.75f); bottom.lineTo (r.getRight() - 8, r.getBottom() - s * 0.75f);
    ice::stoneLine (g, top, s, s * 1.05f, 3);
    ice::stoneLine (g, bottom, s, s * 1.05f, 9);

    g.setColour (goldDark);
    g.drawRoundedRectangle (r, 7.0f, 1.2f);
}

void BlingLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.setColour (goldLight.withAlpha (0.6f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, 1), juce::Justification::centred);
    g.setColour (black);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void BlingLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (black);
    g.fillRoundedRectangle (r, 6.0f);
    juce::Path border; border.addRoundedRectangle (r.reduced (2.5f), 5.0f);
    g.setGradientFill (ice::goldGradient (r));
    g.drawRoundedRectangle (r, 6.0f, 2.0f);
    ice::stoneLine (g, border, 4.6f, 5.0f, 11);

    juce::Path arrow;
    const float ax = r.getRight() - 18.0f, ay = r.getCentreY();
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour (gold);
    g.fillPath (arrow);
}

juce::Font BlingLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::FontOptions (14.0f, juce::Font::bold); }

void BlingLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 3, box.getWidth() - 34, box.getHeight() - 6);
    label.setFont (getComboBoxFont (box));
}

juce::Font BlingLookAndFeel::getLabelFont (juce::Label& l)
{
    return juce::FontOptions (juce::jmax (11.0f, l.getFont().getHeight()), juce::Font::bold);
}

void BlingLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (panel);
    g.setColour (goldMid);
    g.drawRect (0, 0, w, h, 1);
}

//==============================================================================
void WaveformView::setData (std::vector<float> d)
{
    data = std::move (d);
    peak = 0.0f;
    for (float v : data) peak = juce::jmax (peak, std::abs (v));
    rebuild();
}

void WaveformView::setFlash (float f)
{
    if (std::abs (f - flash) > 0.01f || ((f <= 0.0f) != (flash <= 0.0f)))
    {
        flash = f;
        repaint();
    }
}

void WaveformView::rebuild()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;
    const float scale = 2.0f;
    cache = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (cache);
    g.addTransform (juce::AffineTransform::scale (scale));

    auto r = getLocalBounds().toFloat();
    g.setColour (black);
    g.fillRoundedRectangle (r, 9.0f);

    auto plot = r.reduced (18.0f, 14.0f);
    const float mid = plot.getCentreY();

    if (! data.empty() && peak > 0.0f)
    {
        const int W = (int) plot.getWidth();
        const float sc = 1.0f / juce::jmax (peak, 0.05f);
        std::vector<float> env ((size_t) W);
        for (int px = 0; px < W; ++px)
        {
            const size_t a = (size_t) px * data.size() / (size_t) W, b = (size_t) (px + 1) * data.size() / (size_t) W;
            float m = 0.0f;
            for (size_t i = a; i < b && i < data.size(); ++i) m = juce::jmax (m, std::abs (data[i]));
            env[(size_t) px] = juce::jmax (0.012f, m * sc);
        }
        for (int pass = 0; pass < 2; ++pass)   // smooth a little so the iced shape reads clean
            for (int px = 1; px < W - 1; ++px)
                env[(size_t) px] = juce::jmax (env[(size_t) px], 0.5f * (env[(size_t) px - 1] + env[(size_t) px + 1]));

        juce::Path fill;
        fill.startNewSubPath (plot.getX(), mid);
        for (int px = 0; px < W; ++px)
            fill.lineTo (plot.getX() + (float) px, mid - env[(size_t) px] * plot.getHeight() * 0.48f);
        for (int px = W - 1; px >= 0; --px)
            fill.lineTo (plot.getX() + (float) px, mid + env[(size_t) px] * plot.getHeight() * 0.48f);
        fill.closeSubPath();

        g.setColour (iceBlue.withAlpha (0.12f));
        g.strokePath (fill, juce::PathStrokeType (10.0f));
        ice::pave (g, fill, 7.5f, 42);
        g.setGradientFill (ice::goldGradient (plot));
        g.strokePath (fill, juce::PathStrokeType (1.8f));
    }

    g.setColour (gold);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("PEAK " + juce::String (juce::Decibels::gainToDecibels (peak, -96.0f), 1) + " dB",
                r.reduced (20, 12), juce::Justification::topRight);
    g.setColour (chromeDim);
    g.drawText ("0.6 s", r.reduced (20, 12), juce::Justification::bottomRight);

    // diamond-lined frame
    juce::Path frame; frame.addRoundedRectangle (r.reduced (4.0f), 7.0f);
    g.setGradientFill (ice::goldGradient (r));
    g.drawRoundedRectangle (r.reduced (1.0f), 9.0f, 2.0f);
    g.drawRoundedRectangle (r.reduced (7.5f), 5.0f, 1.0f);
    ice::stoneLine (g, frame, 5.6f, 6.0f, 77);
    repaint();
}

void WaveformView::paint (juce::Graphics& g)
{
    if (cache.isValid())
        g.drawImage (cache, getLocalBounds().toFloat());

    if (flash > 0.01f)
    {
        auto r = getLocalBounds().toFloat();
        juce::ColourGradient fg (juce::Colours::white.withAlpha (0.22f * flash), r.getX() + r.getWidth() * 0.2f, r.getCentreY(),
                                 iceBlue.withAlpha (0.0f), r.getRight(), r.getCentreY(), true);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (r.reduced (8.0f), 6.0f);
    }
}

//==============================================================================
TrapSnareEditor::TrapSnareEditor (TrapSnareProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    auto section = [this] (const juce::String& title, std::initializer_list<std::pair<const char*, const char*>> items,
                           std::initializer_list<juce::Component*> extras = {})
    {
        Section s { title, {}, extras, {} };
        for (auto& [id, text] : items)
            s.knobs.push_back (&addKnob (id, text));
        sections.push_back (std::move (s));
    };

    section ("BODY",   { { "tune", "TUNE" }, { "drop", "DROP" }, { "dropTime", "DROP TIME" }, { "bodyDecay", "DECAY" }, { "bodyLevel", "LEVEL" } });
    section ("NOISE",  { { "noiseTone", "TONE" }, { "noiseDecay", "DECAY" }, { "snap", "SNAP" }, { "noiseLevel", "LEVEL" } });
    section ("CLAP",   { { "clapLevel", "LEVEL" }, { "clapTone", "TONE" }, { "clapSpread", "SPREAD" }, { "clapDecay", "DECAY" } });
    section ("FX",     { { "drive", "DRIVE" }, { "reverb", "REVERB" }, { "room", "ROOM" } });
    section ("ROLL",   { { "rollRamp", "RAMP" }, { "rollPitch", "PITCH" } }, { &rollToggle, &rollRate });
    section ("MASTER", { { "variation", "VARIATION" }, { "velSens", "VELOCITY" }, { "level", "LEVEL" } }, { &keyTrackToggle });

    rollRate.addItemList (ts::rollRateNames(), 1);
    rollRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "rollRate", rollRate);
    rollAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "roll", rollToggle);
    keyTrackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "keyTrack", keyTrackToggle);
    rollToggle.setTooltip ("Hold a key to roll at the selected rate (synced to your tempo)");
    keyTrackToggle.setTooltip ("Play the snare chromatically across the keyboard (C3 = original pitch)");
    for (auto* c : std::initializer_list<juce::Component*> { &rollToggle, &rollRate, &keyTrackToggle })
        addAndMakeVisible (c);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0)
        {
            proc.setCurrentProgram (presetBox.getSelectedId() - 1);
            proc.audition();
        }
    };
    addAndMakeVisible (presetBox);

    randomButton.onClick = [this]
    {
        proc.randomize();
        presetBox.setText ("Randomized", juce::dontSendNotification);
        proc.audition();
    };
    hitButton.onClick = [this] { proc.audition(); };
    addAndMakeVisible (randomButton);
    addAndMakeVisible (hitButton);
    addAndMakeVisible (waveform);

    juce::Random r (7);
    for (int i = 0; i < 64; ++i)
        sparkles.push_back ({ { 0, 0 }, r.nextFloat() * juce::MathConstants<float>::twoPi,
                              0.6f + r.nextFloat() * 1.8f, 4.0f + r.nextFloat() * 9.0f });

    setSize (1090, 680);
    startTimerHz (30);
    timerCallback();
}

TrapSnareEditor::~TrapSnareEditor()
{
    setLookAndFeel (nullptr);
}

TrapSnareEditor::Knob& TrapSnareEditor::addKnob (const juce::String& paramId, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxTextColourId, gold);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k->slider);
    if (auto* param = proc.apvts.getParameter (paramId))
        k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    k->label.setColour (juce::Label::textColourId, chrome);

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

//==============================================================================
void TrapSnareEditor::renderBackground()
{
    const float scale = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    const auto full = getLocalBounds().toFloat();

    // velvet black with a warm centre glow + faint diamond-plate lattice
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1710), full.getCentreX(), full.getHeight() * 0.35f,
                                             black, full.getRight(), full.getBottom(), true));
    g.fillAll();
    g.setColour (gold.withAlpha (0.035f));
    for (float d = -full.getHeight(); d < full.getWidth(); d += 26.0f)
    {
        g.drawLine (d, 0, d + full.getHeight(), full.getHeight(), 1.0f);
        g.drawLine (d + full.getHeight(), 0, d, full.getHeight(), 1.0f);
    }

    // iced-out tennis chain frame: every link is a square gold setting holding a stone
    {
        auto frame = full.reduced (11.0f);
        juce::Path rail; rail.addRectangle (frame);
        const float len = rail.getLength(), step = 19.0f;
        int i = 0;
        for (float d = 0; d < len; d += step, ++i)
        {
            const auto pnt = rail.getPointAlongPath (d);
            auto box = juce::Rectangle<float> (18.0f, 18.0f).withCentre (pnt);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (box.translated (1.0f, 2.0f), 4.0f);
            g.setGradientFill (juce::ColourGradient (goldLight, box.getX(), box.getY(), goldDark, box.getRight(), box.getBottom(), false));
            g.fillRoundedRectangle (box, 4.0f);
            ice::drawStone (g, pnt, 15.5f, i & 3);
        }
        for (auto corner : { frame.getTopLeft(), frame.getTopRight(), frame.getBottomLeft(), frame.getBottomRight() })
            ice::drawGem (g, juce::Rectangle<float> (30, 24).withCentre (corner), 1.0f);
    }

    // section panels with diamond-channel borders
    uint32_t seed = 100;
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0, 3), 11.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17151b), r.getX(), r.getY(),
                                                 juce::Colour (0xff0b0a0d), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 11.0f);

        g.setGradientFill (ice::goldGradient (r));
        g.drawRoundedRectangle (r.reduced (1.0f), 11.0f, 2.0f);
        g.drawRoundedRectangle (r.reduced (9.0f), 4.0f, 1.0f);
        juce::Path channel; channel.addRoundedRectangle (r.reduced (5.0f), 7.5f);
        ice::stoneLine (g, channel, 6.6f, 7.0f, ++seed);

        auto tr = s.bounds.reduced (20, 14).removeFromTop (18).toFloat();
        ice::drawGem (g, tr.removeFromLeft (16).reduced (0, 1), 0.4f);
        tr.removeFromLeft (7);
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::FontOptions (16.0f, juce::Font::bold), s.title, tr.getX(), tr.getBottom() - 3.0f);
        juce::Path tp; ga.createPath (tp);
        g.setColour (juce::Colours::black);
        g.strokePath (tp, juce::PathStrokeType (3.0f));
        g.setGradientFill (ice::goldGradient (tp.getBounds()));
        g.fillPath (tp);
    }

    // title: letters fully set with pavé
    {
        const auto tb = titlePath.getBounds();
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillPath (titlePath, juce::AffineTransform::translation (3.0f, 5.0f));
        g.setGradientFill (ice::goldGradient (tb));
        g.strokePath (titlePath, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        ice::pave (g, titlePath, 6.6f, 2024);
        g.setColour (goldLight);
        g.strokePath (titlePath, juce::PathStrokeType (1.2f));

        g.setColour (chrome);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("F U L L Y   B U S S E D   D O W N   D R U M   S Y N T H",
                    juce::Rectangle<float> (tb.getX(), tb.getBottom() + 9.0f, 460.0f, 14.0f), juce::Justification::centredLeft);
    }
}

void TrapSnareEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    const auto tb = titlePath.getBounds();
    {   // shine sweep across the iced title
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (titlePath);
        const float sx = tb.getX() - 120.0f + (tb.getWidth() + 240.0f) * shimmer;
        juce::ColourGradient shine (juce::Colours::white.withAlpha (0.0f), sx - 45.0f, tb.getY(),
                                    juce::Colours::white.withAlpha (0.0f), sx + 45.0f, tb.getBottom(), false);
        shine.addColour (0.5, juce::Colours::white.withAlpha (0.75f));
        g.setGradientFill (shine);
        g.fillRect (tb.expanded (4.0f));
    }

    const float glow = 0.6f + 0.4f * flash;
    ice::drawGem (g, juce::Rectangle<float> (40, 33).withCentre ({ tb.getX() - 32.0f, tb.getCentreY() }), glow);
    ice::drawGem (g, juce::Rectangle<float> (40, 33).withCentre ({ tb.getRight() + 34.0f, tb.getCentreY() }), glow);
}

void TrapSnareEditor::paintOverChildren (juce::Graphics& g)
{
    for (auto& s : sparkles)
    {
        const float a = std::pow (juce::jmax (0.0f, std::sin (s.phase)), 6.0f);
        ice::drawSparkle (g, s.pos, s.size * (0.4f + 0.6f * a), a);
    }
}

void TrapSnareEditor::resized()
{
    auto area = getLocalBounds().reduced (30, 30);

    auto header = area.removeFromTop (88);
    titleArea = header.withWidth (580);
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::FontOptions (58.0f, juce::Font::bold), "TRAP SNARE",
                          (float) titleArea.getX() + 58.0f, (float) titleArea.getY() + 54.0f);
        titlePath.clear();
        ga.createPath (titlePath);
    }
    auto right = header.removeFromRight (440).withSizeKeepingCentre (440, 42);
    hitButton.setBounds (right.removeFromRight (110));
    right.removeFromRight (10);
    randomButton.setBounds (right.removeFromRight (130));
    right.removeFromRight (10);
    presetBox.setBounds (right);

    area.removeFromTop (6);
    waveform.setBounds (area.removeFromTop (126));
    area.removeFromTop (12);

    const int gap = 12, rowH = (area.getHeight() - gap) / 2, knobW = 70, extraW = 136;
    auto row1 = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto row2 = area;

    auto layout = [&] (juce::Rectangle<int> row, std::vector<int> idx)
    {
        for (size_t n = 0; n < idx.size(); ++n)
        {
            auto& s = sections[(size_t) idx[n]];
            const bool last = n + 1 == idx.size();
            const int w = last ? row.getWidth()
                               : (int) s.knobs.size() * knobW + (s.extras.empty() ? 0 : extraW) + 34;
            s.bounds = row.removeFromLeft (w);
            row.removeFromLeft (gap);

            auto inner = s.bounds.reduced (17, 14);
            inner.removeFromTop (22);
            if (! s.extras.empty())
            {
                auto col = inner.removeFromLeft (extraW).reduced (0, 12);
                const int eh = 32;
                auto stack = col.withSizeKeepingCentre (col.getWidth() - 8, (int) s.extras.size() * (eh + 10));
                for (auto* c : s.extras)
                {
                    c->setBounds (stack.removeFromTop (dynamic_cast<juce::ComboBox*> (c) ? eh + 2 : eh));
                    stack.removeFromTop (10);
                }
            }
            for (auto* k : s.knobs)
            {
                auto cell = inner.removeFromLeft (knobW);
                k->label.setBounds (cell.removeFromBottom (16));
                k->slider.setBounds (cell);
            }
        }
    };
    layout (row1, { 0, 1, 2 });
    layout (row2, { 3, 4, 5 });

    // sparkles: on knobs, the title, the waveform and the chain
    juce::Random r (99);
    std::vector<juce::Point<float>> spots;
    for (auto& k : knobs)
        spots.push_back (k->slider.getBounds().toFloat().getCentre() + juce::Point<float> (r.nextFloat() * 40 - 20, r.nextFloat() * 34 - 22));
    const auto tb = titlePath.getBounds();
    for (int i = 0; i < 12; ++i)
        spots.push_back ({ tb.getX() + r.nextFloat() * tb.getWidth(), tb.getY() + r.nextFloat() * tb.getHeight() });
    const auto wb = waveform.getBounds().toFloat();
    for (int i = 0; i < 10; ++i)
        spots.push_back ({ wb.getX() + 20 + r.nextFloat() * wb.getWidth() * 0.5f, wb.getY() + 25 + r.nextFloat() * (wb.getHeight() - 50) });
    for (int i = 0; i < 18; ++i)
        spots.push_back ({ 20.0f + r.nextFloat() * (getWidth() - 40.0f), r.nextBool() ? 11.0f : getHeight() - 11.0f });
    for (size_t i = 0; i < sparkles.size(); ++i)
        sparkles[i].pos = spots[i % spots.size()];

    renderBackground();
}

void TrapSnareEditor::timerCallback()
{
    const auto p = proc.readParams();
    if (firstUpdate || p != lastParams)
    {
        waveform.setData (ts::Engine::renderPreview (p, 22050.0, 0.6));
        lastParams = p;
        firstUpdate = false;
    }

    const auto hits = proc.getHitCount();
    if (hits != lastHits) { lastHits = hits; flash = 1.0f; }
    else                  flash = flash < 0.01f ? 0.0f : flash * 0.85f;
    waveform.setFlash (flash);

    shimmer += 1.0f / 75.0f;
    if (shimmer > 1.0f) shimmer -= 1.0f;
    repaint (titlePath.getBounds().expanded (60.0f, 10.0f).getSmallestIntegerContainer());

    const float dt = 1.0f / 30.0f;
    for (auto& s : sparkles)
    {
        s.phase += s.speed * dt * (1.0f + 2.0f * flash);
        if (s.phase > juce::MathConstants<float>::twoPi) s.phase -= juce::MathConstants<float>::twoPi;
        const float rr = s.size * 1.5f + 2.0f;
        repaint (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (s.pos).getSmallestIntegerContainer());
    }
}
