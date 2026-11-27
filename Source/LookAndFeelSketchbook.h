#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>
#include <cmath>

namespace sylredhel::sketch
{
    // Shared Sylredhel warm cream sketchbook paper & graphite palette
    static inline const juce::Colour kPaperTop    { 0xfff7f2e6 };
    static inline const juce::Colour kPaperBot    { 0xffece3cf };
    static inline const juce::Colour kCardFill    { 0xfffaf6ec };
    static inline const juce::Colour kPlotBg      { 0xfff3ecd9 };
    static inline const juce::Colour kGraphite    { 0xff2f2c28 };
    static inline const juce::Colour kGraphiteMid { 0xff5a544b };
    static inline const juce::Colour kGraphiteLt  { 0xff9b9283 };
    static inline const juce::Colour kGridLine    { 0xffdfd6c1 };

    // Slippery Fairy muted colored-pencil accents
    static inline const juce::Colour kCerulean    { 0xff2f6f99 };
    static inline const juce::Colour kSageTeal    { 0xff2f8578 };
    static inline const juce::Colour kOchre       { 0xffc0842c };
    static inline const juce::Colour kLavender    { 0xff6d5a9e };
    static inline const juce::Colour kTerracotta  { 0xffc6523c };

    static inline void drawSketchBox(juce::Graphics& g, juce::Rectangle<float> r,
                                     float corner = 6.0f, juce::Colour strokeCol = kGraphiteMid)
    {
        g.setColour(strokeCol.withAlpha(0.78f));
        g.drawRoundedRectangle(r, corner, 1.35f);
        g.setColour(strokeCol.withAlpha(0.24f));
        g.drawRoundedRectangle(r.translated(0.9f, 0.7f).reduced(0.4f), corner, 0.9f);
    }

    static inline void drawPencilHatch(juce::Graphics& g, juce::Rectangle<float> r,
                                       juce::Colour col, float step = 7.0f)
    {
        g.saveState();
        g.reduceClipRegion(r.toNearestInt());
        g.setColour(col);
        const float h = r.getHeight();
        for (float x = r.getX() - h; x < r.getRight() + h; x += step)
            g.drawLine(x, r.getBottom(), x + h, r.getY(), 0.7f);
        g.restoreState();
    }
}

class LookAndFeelSketchbook : public juce::LookAndFeel_V4
{
public:
    static constexpr const char* kAccentProp  = "sylAccent";
    static constexpr const char* kBipolarProp = "sylBipolar";

    LookAndFeelSketchbook()
    {
        using namespace sylredhel::sketch;
        setColour(juce::ComboBox::backgroundColourId, kCardFill);
        setColour(juce::ComboBox::textColourId,       kGraphite);
        setColour(juce::ComboBox::outlineColourId,    kGraphiteMid);
        setColour(juce::ComboBox::arrowColourId,      kGraphite);

        setColour(juce::PopupMenu::backgroundColourId,            kPaperTop);
        setColour(juce::PopupMenu::textColourId,                  kGraphite);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, kCerulean.withAlpha(0.22f));
        setColour(juce::PopupMenu::highlightedTextColourId,       kGraphite);

        setColour(juce::TextButton::buttonColourId,   kCardFill);
        setColour(juce::TextButton::buttonOnColourId, kCerulean.withAlpha(0.25f));
        setColour(juce::TextButton::textColourOffId,  kGraphite);
        setColour(juce::TextButton::textColourOnId,   kGraphite);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        using namespace sylredhel::sketch;
        auto b = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
        g.setColour(box.isMouseOver() ? juce::Colour(0xfffffbf2) : kCardFill);
        g.fillRoundedRectangle(b, 4.5f);
        drawSketchBox(g, b, 4.5f, box.isMouseOver() ? kGraphite : kGraphiteMid);

        const float cx = static_cast<float>(buttonX) + static_cast<float>(buttonW) * 0.5f;
        const float cy = static_cast<float>(buttonY) + static_cast<float>(buttonH) * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
        arrow.lineTo(cx, cy + 2.8f);
        arrow.lineTo(cx + 4.0f, cy - 2.0f);
        g.setColour(kGraphite);
        g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour&, bool isHighlighted, bool isDown) override
    {
        using namespace sylredhel::sketch;
        auto b = btn.getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(isDown ? kGridLine : (isHighlighted ? juce::Colour(0xfffffbf2) : kCardFill));
        g.fillRoundedRectangle(b, 4.5f);
        drawSketchBox(g, b, 4.5f, isHighlighted ? kGraphite : kGraphiteMid);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        using namespace sylredhel::sketch;
        const auto b = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                              static_cast<float>(width), static_cast<float>(height));
        const float cx = b.getCentreX();
        const float cy = b.getCentreY() + 1.0f;
        const float r  = juce::jmin(b.getWidth(), b.getHeight()) * 0.34f;
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        const uint32_t colHex = static_cast<uint32_t>(
            static_cast<int>(slider.getProperties().getWithDefault(kAccentProp, static_cast<int>(0xff2f6f99))));
        const bool bipolar = static_cast<bool>(slider.getProperties().getWithDefault(kBipolarProp, false));
        const juce::Colour accent(colHex);

        // Hand-sketched dial ticks
        const float tickR1 = r + 3.5f;
        const float tickR2 = r + 6.5f;
        g.setColour(kGraphiteLt.withAlpha(0.75f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (static_cast<float>(i) / 10.0f) * (endAngle - startAngle) - juce::MathConstants<float>::halfPi;
            g.drawLine(cx + std::cos(a) * tickR1, cy + std::sin(a) * tickR1,
                       cx + std::cos(a) * tickR2, cy + std::sin(a) * tickR2, (i == 0 || i == 5 || i == 10) ? 1.4f : 0.9f);
        }

        // Colored-pencil value arc (bipolar centered at 12 o'clock when enabled)
        const float arcR = r + 5.0f;
        const float midAngle = 0.5f * (startAngle + endAngle);
        if (bipolar)
        {
            if (std::abs(angle - midAngle) > 0.01f)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, arcR, arcR, 0.0f,
                                     std::min(midAngle, angle), std::max(midAngle, angle), true);
                g.setColour(accent.withAlpha(0.88f));
                g.strokePath(valArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(accent.withAlpha(0.28f));
                g.strokePath(valArc, juce::PathStrokeType(4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour(kGraphite);
            g.drawVerticalLine(static_cast<int>(cx), cy - arcR - 4.0f, cy - arcR + 1.5f);
        }
        else if (sliderPos > 0.002f)
        {
            juce::Path valArc;
            valArc.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour(accent.withAlpha(0.88f));
            g.strokePath(valArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(accent.withAlpha(0.28f));
            g.strokePath(valArc, juce::PathStrokeType(4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Soft paper-pencil drop shadow
        g.setColour(kGraphite.withAlpha(0.10f));
        g.fillEllipse(cx - r + 1.8f, cy - r + 2.4f, r * 2.0f, r * 2.0f);

        // Cream sketchbook knob face with double-pass graphite outline
        juce::ColourGradient cap(juce::Colour(0xfffffcf5), cx - r * 0.4f, cy - r * 0.5f,
                                 juce::Colour(0xffe9dfc9), cx + r * 0.6f, cy + r * 0.6f, false);
        g.setGradientFill(cap);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        g.setColour(kGraphite.withAlpha(0.85f));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
        g.setColour(kGraphite.withAlpha(0.28f));
        g.drawEllipse(cx - r + 0.8f, cy - r - 0.5f, r * 2.0f - 0.5f, r * 2.0f + 0.3f, 0.9f);

        // Bold colored-pencil pointer line
        const float ptrA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(accent);
        g.drawLine(cx + std::cos(ptrA) * (r * 0.22f), cy + std::sin(ptrA) * (r * 0.22f),
                   cx + std::cos(ptrA) * (r * 0.84f), cy + std::sin(ptrA) * (r * 0.84f), 2.5f);
        g.setColour(kGraphite);
        g.fillEllipse(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
                          bool isHighlighted, bool) override
    {
        using namespace sylredhel::sketch;
        auto b = btn.getLocalBounds().toFloat().reduced(1.5f);
        const bool isOn = btn.getToggleState();
        const bool isBypass = btn.getButtonText().containsIgnoreCase("BYPASS");

        const juce::Colour fillCol = isOn
            ? (isBypass ? kTerracotta.withAlpha(0.26f) : kCerulean.withAlpha(0.22f))
            : (isHighlighted ? juce::Colour(0xfffffbf2) : kCardFill);
        const juce::Colour rimCol = isOn
            ? (isBypass ? kTerracotta : kCerulean)
            : (isHighlighted ? kGraphite : kGraphiteMid);

        g.setColour(fillCol);
        g.fillRoundedRectangle(b, 4.5f);
        if (isOn)
            drawPencilHatch(g, b.reduced(1.5f), rimCol.withAlpha(0.18f), 6.0f);
        drawSketchBox(g, b, 4.5f, rimCol);

        g.setColour(isOn ? (isBypass ? kTerracotta.darker(0.35f) : kCerulean.darker(0.30f)) : kGraphite);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(btn.getButtonText(), b.toNearestInt(), juce::Justification::centred);
    }
};

/**
 * Modular Hand-Drawn Sketchbook Slippery Fairy Mascot!
 *
 * A gentle, squishy dewdrop / rain-sprite critter trying to stay balanced on a
 * glossy little sketchbook puddle. Physically reacts to BOTH:
 *   - `slipActivity` [0..1]: squashes & stretches, waves tiny pencil arms, kicks up water droplets!
 *   - `asymmetry` [-1..+1]: tilts & skids right (+asym slow fall), left (-asym slow rise),
 *                           or wobbles symmetrically like jelly (0%)!
 *
 * Modular: replace paint() with your own PNG sprite frames later if desired.
 */
class SlipperyFairyMascot : public juce::Component
{
public:
    void updateActivity(float slipActivityIn, float asymPctIn)
    {
        const float targetAct  = juce::jlimit(0.0f, 1.0f, slipActivityIn);
        const float targetAsym = juce::jlimit(-1.0f, 1.0f, asymPctIn * 0.01f);

        activity = activity * 0.76f + targetAct * 0.24f;
        asym     = asym     * 0.82f + targetAsym * 0.18f;

        phase += 0.13f + 0.30f * activity;
        if (phase > 6.283185f) phase -= 6.283185f;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace sylredhel::sketch;
        auto b = getLocalBounds().toFloat();

        // Warm sketchbook portrait card + little corner masking tape
        g.setColour(kPlotBg);
        g.fillRoundedRectangle(b, 6.0f);
        drawSketchBox(g, b.reduced(0.6f), 6.0f, kGraphiteMid);

        g.setColour(kCerulean.withAlpha(0.20f));
        g.fillRect(b.getCentreX() - 22.0f, b.getY() - 1.0f, 44.0f, 10.0f);

        // Physics-driven skid offset, shear lean, and squash & stretch!
        const float wobble   = std::sin(phase);
        const float wobble2  = std::sin(phase * 2.0f);
        const float skidDir  = (std::abs(asym) > 0.12f) ? asym : (0.55f * wobble);
        const float slideX   = skidDir * activity * 15.0f + wobble * (1.2f + 3.5f * activity);
        const float leanTopX = skidDir * activity * 11.0f + wobble * (0.8f + 2.2f * activity);

        const float squash   = 0.05f * std::sin(phase) + 0.14f * activity * (0.6f + 0.4f * wobble2);
        const float scaleX   = 1.0f + squash;
        const float scaleY   = 1.0f - squash * 0.75f;

        const float baseY = b.getBottom() - 40.0f;
        const float cx    = b.getCentreX() + slideX;

        // 1. Glossy Sketchbook Puddle / Ice Patch underneath
        const float pudX = b.getCentreX();
        const float pudW = 108.0f + 10.0f * activity;
        g.setColour(kCerulean.withAlpha(0.15f + 0.08f * activity));
        g.fillEllipse(pudX - pudW * 0.5f, baseY - 6.0f, pudW, 17.0f);
        g.setColour(kCerulean.withAlpha(0.55f));
        g.drawEllipse(pudX - pudW * 0.5f, baseY - 6.0f, pudW, 17.0f, 1.15f);
        // Little puddle gloss shine streaks
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawLine(pudX - 28.0f, baseY + 1.0f, pudX - 10.0f, baseY + 1.0f, 1.5f);
        g.drawLine(pudX + 16.0f, baseY + 3.5f, pudX + 27.0f, baseY + 3.5f, 1.2f);

        // Pencil skid-trail lines behind feet when slipping!
        if (activity > 0.10f)
        {
            const float trailSign = (skidDir >= 0.0f) ? -1.0f : 1.0f;
            const float tx = cx + trailSign * 22.0f;
            g.setColour(kCerulean.withAlpha(std::min(0.75f, activity * 1.1f)));
            for (int i = 0; i < 3; ++i)
            {
                const float dy = -3.0f + 3.2f * static_cast<float>(i);
                const float len = (10.0f + 10.0f * activity) - 2.5f * static_cast<float>(i);
                g.drawLine(tx, baseY + dy, tx + trailSign * len, baseY + dy + 0.8f, 1.2f);
            }
        }

        // 2. Tiny Slipping Feet (kicking slightly up when sliding!)
        const float footLiftL = std::max(0.0f, -skidDir) * activity * 7.0f + std::max(0.0f, wobble2) * activity * 3.0f;
        const float footLiftR = std::max(0.0f,  skidDir) * activity * 7.0f + std::max(0.0f, -wobble2) * activity * 3.0f;

        g.setColour(kGraphite);
        g.drawLine(cx - 10.0f, baseY - 4.0f, cx - 16.0f, baseY + 1.0f - footLiftL, 1.6f);
        g.drawLine(cx + 10.0f, baseY - 4.0f, cx + 16.0f, baseY + 1.0f - footLiftR, 1.6f);

        // 3. Squishy Translucent Dewdrop / Jelly-Blob Body (sheared by leanTopX!)
        const float bodyW = 28.0f * scaleX;
        const float bodyH = 58.0f * scaleY;
        const float topX  = cx + leanTopX;
        const float topY  = baseY - bodyH;

        juce::Path blob;
        blob.startNewSubPath(cx - bodyW, baseY - 5.0f);
        // Smooth organic raindrop/jelly dome that leans asymmetrically with slew polarity!
        blob.cubicTo(cx - bodyW * 1.08f, baseY - bodyH * 0.42f,
                     topX - bodyW * 0.62f, topY + bodyH * 0.14f,
                     topX, topY - 8.0f); // Little cute droplet tip on head!
        blob.cubicTo(topX + bodyW * 0.62f, topY + bodyH * 0.14f,
                     cx + bodyW * 1.08f, baseY - bodyH * 0.42f,
                     cx + bodyW, baseY - 5.0f);
        blob.quadraticTo(cx, baseY + 3.5f, cx - bodyW, baseY - 5.0f);
        blob.closeSubPath();

        // Soft watercolor cerulean-cream gradient fill
        juce::ColourGradient blobFill(juce::Colour(0xfffffdf7), topX - 10.0f, topY,
                                      kCerulean.withAlpha(0.24f), cx + 14.0f, baseY, false);
        g.setGradientFill(blobFill);
        g.fillPath(blob);

        // Glossy jelly highlight arc inside forehead
        juce::Path gloss;
        gloss.startNewSubPath(topX - 11.0f * scaleX, topY + 14.0f * scaleY);
        gloss.quadraticTo(topX - 6.0f * scaleX, topY + 5.0f * scaleY,
                          topX + 1.0f * scaleX, topY + 6.0f * scaleY);
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.strokePath(gloss, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Double-pass hand-sketched graphite contour
        g.setColour(kGraphite);
        g.strokePath(blob, juce::PathStrokeType(1.65f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(kGraphite.withAlpha(0.25f));
        g.strokePath(blob, juce::PathStrokeType(0.95f), juce::AffineTransform::translation(0.8f, 0.5f));

        // 4. Tiny Flailing Pencil Arms (trying to keep balance on the puddle!)
        const float armWave = std::sin(phase * 2.2f) * (3.0f + 11.0f * activity);
        const float shY = baseY - bodyH * 0.42f;
        const float shX = cx + leanTopX * 0.45f;
        g.setColour(kGraphite);
        g.drawLine(shX - bodyW * 0.82f, shY,
                   shX - bodyW * 1.28f, shY - 4.0f - armWave, 1.5f);
        g.drawLine(shX + bodyW * 0.82f, shY,
                   shX + bodyW * 1.28f, shY - 4.0f + armWave, 1.5f);

        // 5. Shy Watchful Eyes (peeking left toward the Hysteresis Loop & Scope!)
        const float faceX = cx + leanTopX * 0.62f;
        const float faceY = baseY - bodyH * 0.56f;
        const float eyeH  = 8.5f + 2.2f * activity; // Slightly wider startled eyes when slipping hard
        const float lookX = -2.2f + skidDir * 1.4f * activity;
        const float lookY = -0.5f - 1.2f * activity;

        g.setColour(juce::Colours::white);
        g.fillEllipse(faceX - 11.5f, faceY - eyeH * 0.5f, 8.2f, eyeH);
        g.fillEllipse(faceX + 2.5f,  faceY - eyeH * 0.5f, 8.2f, eyeH);
        g.setColour(kGraphite);
        g.drawEllipse(faceX - 11.5f, faceY - eyeH * 0.5f, 8.2f, eyeH, 1.2f);
        g.drawEllipse(faceX + 2.5f,  faceY - eyeH * 0.5f, 8.2f, eyeH, 1.2f);

        g.fillEllipse(faceX - 8.2f + lookX, faceY - 1.6f + lookY, 3.6f, 3.8f);
        g.fillEllipse(faceX + 5.8f + lookX, faceY - 1.6f + lookY, 3.6f, 3.8f);

        // Soft blush & worried/shy mouth
        g.setColour(kTerracotta.withAlpha(0.22f + 0.16f * activity));
        g.fillEllipse(faceX - 15.5f, faceY + 5.0f, 6.5f, 3.8f);
        g.fillEllipse(faceX + 8.5f,  faceY + 5.0f, 6.5f, 3.8f);

        g.setColour(kGraphite);
        if (activity > 0.35f)
        {
            // Tiny surprised wavy 'o' mouth when skidding!
            g.drawEllipse(faceX - 2.3f, faceY + 6.0f, 4.6f, 5.0f, 1.25f);
        }
        else
        {
            // Gentle shy smile/line
            juce::Path smile;
            smile.startNewSubPath(faceX - 3.5f, faceY + 7.5f);
            smile.quadraticTo(faceX, faceY + 9.8f, faceX + 3.5f, faceY + 7.5f);
            g.strokePath(smile, juce::PathStrokeType(1.25f));
        }

        // 6. Floating Water / Sweat Droplets when slipping actively!
        if (activity > 0.14f)
        {
            const float dropAlpha = std::min(0.90f, activity * 1.35f);
            auto drawTinyDrop = [&](float dx, float dy, float sz)
            {
                juce::Path d;
                d.startNewSubPath(dx, dy - sz);
                d.quadraticTo(dx + sz * 0.85f, dy + sz * 0.4f, dx, dy + sz);
                d.quadraticTo(dx - sz * 0.85f, dy + sz * 0.4f, dx, dy - sz);
                d.closeSubPath();
                g.setColour(kCerulean.withAlpha(dropAlpha * 0.35f));
                g.fillPath(d);
                g.setColour(kCerulean.withAlpha(dropAlpha));
                g.strokePath(d, juce::PathStrokeType(1.1f));
            };

            const float rise1 = std::fmod(phase * 4.0f, 12.0f);
            const float rise2 = std::fmod(phase * 4.0f + 6.0f, 12.0f);
            drawTinyDrop(topX + 24.0f, topY + 12.0f - rise1, 4.2f);
            drawTinyDrop(topX - 26.0f, topY + 20.0f - rise2, 3.4f);
        }

        // Caption at bottom of portrait
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::italic)));
        g.setColour(kGraphiteMid);
        const juce::String mood = (activity > 0.42f) ? "whoa, slippery!"
                                : (activity > 0.10f) ? "sliding smoothly..."
                                                     : "balancing quietly";
        g.drawText(mood, static_cast<int>(b.getX() + 6), static_cast<int>(b.getBottom() - 20),
                   static_cast<int>(b.getWidth() - 12), 14, juce::Justification::centred);
    }

private:
    float activity = 0.0f;
    float asym     = 0.65f;
    float phase    = 0.0f;
};

/**
 * Sketchbook Graph-Paper Dual-Trace Slip Oscilloscope.
 */
class SlipOscilloscopeView : public juce::Component
{
public:
    explicit SlipOscilloscopeView(SlipperyFairyAudioProcessor& proc) : processorRef(proc) {}

    void updateFrame(int modeIdx, int osModeIdx)
    {
        slipModeIdx = modeIdx;
        osIdx       = osModeIdx;
        slipAct     = processorRef.getSlipActivity();

        const double fs = std::max(processorRef.getCurrentSampleRate(), 8000.0);
        latencyMs = static_cast<float>(1000.0 * static_cast<double>(processorRef.getEngineLatencySamples()) / fs);

        processorRef.getScope().copyLatest(inTrace.data(), outTrace.data(), kViewSamples);
        repaint();
    }

    const std::array<float, 1024>& getInTrace()  const noexcept { return inTrace; }
    const std::array<float, 1024>& getOutTrace() const noexcept { return outTrace; }

    void paint(juce::Graphics& g) override
    {
        using namespace sylredhel::sketch;
        auto b = getLocalBounds().toFloat();

        g.setColour(kPlotBg);
        g.fillRoundedRectangle(b, 6.0f);
        drawSketchBox(g, b.reduced(0.6f), 6.0f, kGraphiteMid);

        auto plot = b.reduced(12.0f, 10.0f);
        plot.removeFromTop(21.0f);

        float peak = 0.15f;
        for (int i = 0; i < kViewSamples; ++i)
            peak = std::max(peak, std::max(std::abs(inTrace[static_cast<size_t>(i)]),
                                           std::abs(outTrace[static_cast<size_t>(i)])));
        smoothPeak = smoothPeak * 0.85f + std::min(peak * 1.15f, 1.8f) * 0.15f;
        const float midY   = plot.getCentreY();
        const float yScale = (plot.getHeight() * 0.45f) / std::max(smoothPeak, 0.12f);

        auto valToY = [&](float v) -> float
        {
            return juce::jlimit(plot.getY(), plot.getBottom(), midY - v * yScale);
        };

        // Faint graph-paper grid
        g.setColour(kGridLine);
        for (int i = 1; i < 8; ++i)
            g.drawVerticalLine(static_cast<int>(plot.getX() + plot.getWidth() * (static_cast<float>(i) / 8.0f)),
                               plot.getY(), plot.getBottom());
        g.drawHorizontalLine(static_cast<int>(plot.getY() + plot.getHeight() * 0.25f), plot.getX(), plot.getRight());
        g.drawHorizontalLine(static_cast<int>(plot.getY() + plot.getHeight() * 0.75f), plot.getX(), plot.getRight());
        g.setColour(kGraphiteLt.withAlpha(0.55f));
        g.drawHorizontalLine(static_cast<int>(midY), plot.getX(), plot.getRight());

        g.saveState();
        g.reduceClipRegion(plot.toNearestInt());

        int trig = 0;
        for (int i = 1; i < kViewSamples / 2; ++i)
        {
            if (inTrace[static_cast<size_t>(i - 1)] <= 0.0f && inTrace[static_cast<size_t>(i)] > 0.0f)
            {
                trig = i;
                break;
            }
        }
        const int drawCount = kViewSamples / 2;

        juce::Path pIn, pOut;
        for (int i = 0; i < drawCount; ++i)
        {
            const float x  = plot.getX() + (static_cast<float>(i) / static_cast<float>(drawCount - 1)) * plot.getWidth();
            const float yi = valToY(inTrace[static_cast<size_t>(trig + i)]);
            const float yo = valToY(outTrace[static_cast<size_t>(trig + i)]);
            if (i == 0) { pIn.startNewSubPath(x, yi); pOut.startNewSubPath(x, yo); }
            else        { pIn.lineTo(x, yi);          pOut.lineTo(x, yo); }
        }

        // Input waveform (warm ochre colored pencil)
        g.setColour(kOchre.withAlpha(0.55f));
        g.strokePath(pIn, juce::PathStrokeType(1.4f));

        // Slipped Output waveform (bold cerulean ink)
        g.setColour(kCerulean);
        g.strokePath(pOut, juce::PathStrokeType(2.0f));

        g.restoreState();

        // Top Telemetry Strip
        const auto top = juce::Rectangle<float>(b.getX() + 12.0f, b.getY() + 6.0f, b.getWidth() - 24.0f, 18.0f);
        static constexpr const char* modeTags[4] = { "OP-AMP SID", "ASYM RC", "VEL QUAD", "DYN SLIP" };
        static constexpr const char* osTags[5]   = { "1x", "2x", "4x", "8x", "16x" };

        g.setFont(juce::Font(juce::FontOptions(10.2f, juce::Font::bold)));
        g.setColour(kGraphite);
        g.drawText("SLIP SCOPE  |  " + juce::String(modeTags[juce::jlimit(0, 3, slipModeIdx)]),
                   static_cast<int>(top.getX()), static_cast<int>(top.getY()), 195, 18,
                   juce::Justification::centredLeft);

        const bool slipping = slipAct > 0.08f;
        g.setColour(slipping ? kCerulean : kGraphiteLt);
        g.fillEllipse(top.getX() + 198.0f, top.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.drawText("SLIP " + juce::String(static_cast<int>(std::round(slipAct * 100.0f))) + "%",
                   static_cast<int>(top.getX() + 209), static_cast<int>(top.getY()), 74, 18,
                   juce::Justification::centredLeft);

        g.setFont(juce::Font(juce::FontOptions(9.2f, juce::Font::bold)));
        g.setColour(kGraphiteMid);
        g.drawText(juce::String(osTags[juce::jlimit(0, 4, osIdx)]) + " OS | PDC " + juce::String(latencyMs, 2) + " ms",
                   static_cast<int>(top.getRight() - 188), static_cast<int>(top.getY()), 120, 18,
                   juce::Justification::centredRight);

        g.setColour(kOchre);
        g.drawText("IN", static_cast<int>(top.getRight() - 62), static_cast<int>(top.getY()), 26, 18, juce::Justification::centredRight);
        g.setColour(kCerulean);
        g.drawText("OUT", static_cast<int>(top.getRight() - 32), static_cast<int>(top.getY()), 32, 18, juce::Justification::centredRight);
    }

private:
    static constexpr int kViewSamples = 1024;
    SlipperyFairyAudioProcessor& processorRef;
    std::array<float, kViewSamples> inTrace {};
    std::array<float, kViewSamples> outTrace {};
    float smoothPeak  = 0.5f;
    float slipAct     = 0.0f;
    float latencyMs   = 0.54f;
    int   slipModeIdx = 0;
    int   osIdx       = 3;
};

/**
 * Sketchbook Graph-Paper Real-Time Phase-Space Hysteresis Loop (Input X -> Output Y).
 */
class HysteresisLoopView : public juce::Component
{
public:
    void updateFromTraces(const std::array<float, 1024>& xBuf,
                          const std::array<float, 1024>& yBuf)
    {
        xPts = xBuf;
        yPts = yBuf;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace sylredhel::sketch;
        auto b = getLocalBounds().toFloat();

        g.setColour(kPlotBg);
        g.fillRoundedRectangle(b, 6.0f);
        drawSketchBox(g, b.reduced(0.6f), 6.0f, kGraphiteMid);

        g.setFont(juce::Font(juce::FontOptions(9.8f, juce::Font::bold)));
        g.setColour(kGraphite);
        g.drawText("HYSTERESIS (X -> Y)", static_cast<int>(b.getX() + 8), static_cast<int>(b.getY() + 6),
                   static_cast<int>(b.getWidth() - 16), 16, juce::Justification::centred);

        auto plot = b.reduced(14.0f, 12.0f);
        plot.removeFromTop(18.0f);

        const float cx   = plot.getCentreX();
        const float cy   = plot.getCentreY();
        const float half = std::min(plot.getWidth(), plot.getHeight()) * 0.46f;

        g.setColour(kGridLine);
        g.drawHorizontalLine(static_cast<int>(cy), plot.getX(), plot.getRight());
        g.drawVerticalLine(static_cast<int>(cx), plot.getY(), plot.getBottom());

        g.setColour(kGraphiteLt.withAlpha(0.65f));
        g.drawLine(cx - half, cy + half, cx + half, cy - half, 1.0f);

        float pk = 0.12f;
        for (int i = 512; i < 1024; ++i)
            pk = std::max(pk, std::max(std::abs(xPts[static_cast<size_t>(i)]),
                                       std::abs(yPts[static_cast<size_t>(i)])));
        smoothPk = smoothPk * 0.85f + std::min(pk * 1.18f, 1.8f) * 0.15f;
        const float invPk = 1.0f / std::max(smoothPk, 0.10f);

        g.saveState();
        g.reduceClipRegion(plot.toNearestInt());

        constexpr int startIdx = 640;
        constexpr int numSegs  = 383;
        constexpr int chunks   = 4;
        constexpr int chunkLen = numSegs / chunks;

        for (int c = 0; c < chunks; ++c)
        {
            juce::Path seg;
            const int i0 = startIdx + c * chunkLen;
            const int i1 = (c == chunks - 1) ? 1023 : (i0 + chunkLen);
            for (int i = i0; i <= i1; ++i)
            {
                const float px = cx + juce::jlimit(-1.05f, 1.05f, xPts[static_cast<size_t>(i)] * invPk) * half;
                const float py = cy - juce::jlimit(-1.05f, 1.05f, yPts[static_cast<size_t>(i)] * invPk) * half;
                if (i == i0) seg.startNewSubPath(px, py);
                else         seg.lineTo(px, py);
            }
            const float alpha = 0.24f + 0.22f * static_cast<float>(c);
            const juce::Colour col = kSageTeal.interpolatedWith(kCerulean, static_cast<float>(c) / 3.0f);
            g.setColour(col.withAlpha(alpha));
            g.strokePath(seg, juce::PathStrokeType(1.35f + 0.22f * static_cast<float>(c)));
        }

        g.restoreState();

        g.setFont(juce::Font(juce::FontOptions(8.3f, juce::Font::italic)));
        g.setColour(kGraphiteMid);
        g.drawText("Static = Line  |  Slew = Open Loop",
                   static_cast<int>(plot.getX()), static_cast<int>(plot.getBottom() - 10),
                   static_cast<int>(plot.getWidth()), 12, juce::Justification::centred);
    }

private:
    std::array<float, 1024> xPts {};
    std::array<float, 1024> yPts {};
    float smoothPk = 0.5f;
};
