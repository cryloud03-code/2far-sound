#pragma once
#include <JuceHeader.h>
#include "Parameters.h"

namespace Pal
{
    inline const juce::Colour bg(0xff0d0e10), edge(0xff34373d), text(0xffc9ccd2), dim(0xff8b909a),
        orange(0xffff8a1f), red(0xffe5393b), green(0xff45e07a), cyan(0xff3fc8e8),
        yellow(0xffffd23f), purple(0xff9b6cff);
}
inline juce::Font F(float h, int style = juce::Font::plain) { return juce::Font(juce::FontOptions(h, style)); }

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
struct MetalLnF : public juce::LookAndFeel_V4
{
    MetalLnF()
    {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff111215));
        setColour(juce::ComboBox::textColourId, Pal::text);
        setColour(juce::ComboBox::outlineColourId, Pal::edge);
        setColour(juce::ComboBox::arrowColourId, Pal::orange);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff17181c));
        setColour(juce::PopupMenu::textColourId, Pal::text);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float a0, float a1, juce::Slider& s) override
    {
        using namespace juce;
        auto b = Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(5.f);
        const float r = jmin(b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const auto accent = s.findColour(Slider::rotarySliderFillColourId);
        const float ang = a0 + pos * (a1 - a0);

        Path track; track.addCentredArc(c.x, c.y, r, r, 0.f, a0, a1, true);
        g.setColour(Colours::black); g.strokePath(track, PathStrokeType(4.f, PathStrokeType::curved, PathStrokeType::rounded));
        Path val; val.addCentredArc(c.x, c.y, r, r, 0.f, a0, ang, true);
        g.setColour(accent); g.strokePath(val, PathStrokeType(3.f, PathStrokeType::curved, PathStrokeType::rounded));

        const float kr = r * 0.74f;
        g.setColour(Colour(0xff050506)); g.fillEllipse(c.x - kr - 2, c.y - kr - 1, kr * 2 + 4, kr * 2 + 5);
        g.setGradientFill(ColourGradient(Colour(0xffe2e5ea), c.x - kr * 0.5f, c.y - kr * 0.6f,
                                         Colour(0xff2c2e34), c.x + kr * 0.7f, c.y + kr * 0.9f, true));
        g.fillEllipse(c.x - kr, c.y - kr, kr * 2, kr * 2);
        g.setColour(Colour(0xff15161a)); g.drawEllipse(c.x - kr * 0.72f, c.y - kr * 0.72f, kr * 1.44f, kr * 1.44f, 1.5f);
        // moleteado
        for (int i = 0; i < 24; ++i)
        {
            const float a = i * MathConstants<float>::twoPi / 24.f;
            g.setColour(Colours::black.withAlpha(0.25f));
            g.drawLine(c.x + std::sin(a) * kr * 0.9f, c.y - std::cos(a) * kr * 0.9f,
                       c.x + std::sin(a) * kr, c.y - std::cos(a) * kr, 1.f);
        }
        g.setColour(accent);
        g.drawLine(c.x + std::sin(ang) * kr * 0.25f, c.y - std::cos(ang) * kr * 0.25f,
                   c.x + std::sin(ang) * kr * 0.82f, c.y - std::cos(ang) * kr * 0.82f, 3.f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        using namespace juce;
        auto r = b.getLocalBounds().toFloat();
        const float d = jmin(14.f, r.getHeight() - 4.f);
        auto led = Rectangle<float>(r.getX() + 4.f, r.getCentreY() - d * 0.5f, d, d);
        const auto col = b.findColour(ToggleButton::tickColourId);
        const bool on = b.getToggleState();
        if (on)
        {
            g.setGradientFill(ColourGradient(col.withAlpha(0.55f), led.getCentreX(), led.getCentreY(),
                                             col.withAlpha(0.f), led.getCentreX() + d * 1.4f, led.getCentreY(), true));
            g.fillEllipse(led.expanded(d * 0.9f));
        }
        g.setColour(Colours::black); g.fillEllipse(led.expanded(2.f));
        if (on) g.setGradientFill(ColourGradient(col.brighter(0.7f), led.getCentreX(), led.getY(), col.darker(0.3f), led.getCentreX(), led.getBottom(), false));
        else    g.setGradientFill(ColourGradient(Colour(0xff40302f), led.getCentreX(), led.getY(), Colour(0xff1b1414), led.getCentreX(), led.getBottom(), false));
        g.fillEllipse(led);
        g.setColour(hl ? Pal::text : Pal::dim); g.setFont(F(12.f, Font::bold));
        g.drawText(b.getButtonText(), r.withTrimmedLeft(d + 12.f), Justification::centredLeft);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& c, bool hl, bool down) override
    {
        using namespace juce;
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        auto base = down ? c.darker(0.4f) : (hl ? c.brighter(0.15f) : c);
        g.setGradientFill(ColourGradient(base.brighter(0.4f), 0.f, r.getY(), base.darker(0.55f), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle(r, 6.f);
        g.setColour(Colours::black.withAlpha(0.8f)); g.drawRoundedRectangle(r, 6.f, 2.f);
    }
    juce::Font getTextButtonFont(juce::TextButton&, int) override { return F(14.f, juce::Font::bold); }
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob(APVTS& apvts, const char* id, const juce::String& name, juce::Colour accent)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(F(11.f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, Pal::text);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 15);
        s.setRotaryParameters(1.25f * juce::MathConstants<float>::pi, 2.75f * juce::MathConstants<float>::pi, true);
        s.setColour(juce::Slider::rotarySliderFillColourId, accent);
        s.setColour(juce::Slider::textBoxTextColourId, Pal::dim);
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(s); addAndMakeVisible(label);
        att = std::make_unique<APVTS::SliderAttachment>(apvts, id, s);
        if (auto* p = apvts.getParameter(id))
            s.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    }
    void resized() override { auto r = getLocalBounds(); label.setBounds(r.removeFromTop(14)); s.setBounds(r); }
private:
    juce::Slider s; juce::Label label;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

struct LedButton : public juce::ToggleButton
{
    LedButton(APVTS& apvts, const char* id, const juce::String& text, juce::Colour led)
    {
        setButtonText(text); setColour(juce::ToggleButton::tickColourId, led);
        att = std::make_unique<APVTS::ButtonAttachment>(apvts, id, *this);
    }
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

struct Choice : public juce::ComboBox
{
    Choice(APVTS& apvts, const char* id, const juce::StringArray& items)
    {
        addItemList(items, 1);
        att = std::make_unique<APVTS::ComboBoxAttachment>(apvts, id, *this);
    }
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};

//==============================================================================
// Medidor LED (nivel: -60..0 dB ; GR: 0..24 dB). Vertical u horizontal.
class Meter : public juce::Component, private juce::Timer
{
public:
    enum Mode { Level, GainReduction };
    Meter(std::atomic<float>& src, Mode m, bool vertical) : source(src), mode(m), vert(vertical) { startTimerHz(30); }
    ~Meter() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        g.setColour(Colour(0xff050506)); g.fillRoundedRectangle(r, 3.f);
        const int segs = vert ? 36 : 24;
        const float lit = shown * (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const float f = (float) i / (float) segs;
            Colour c = mode == GainReduction ? Pal::orange : (f > 0.9f ? Pal::red : f > 0.72f ? Pal::yellow : Pal::green);
            Rectangle<float> s = vert
                ? Rectangle<float>(r.getX() + 3, r.getBottom() - 3 - (i + 1) * (r.getHeight() - 6) / segs + 1, r.getWidth() - 6, (r.getHeight() - 6) / segs - 1)
                : Rectangle<float>(r.getX() + 3 + i * (r.getWidth() - 6) / segs, r.getY() + 3, (r.getWidth() - 6) / segs - 1, r.getHeight() - 6);
            g.setColour((float) i < lit ? c : c.withAlpha(0.12f));
            g.fillRect(s);
        }
    }
private:
    void timerCallback() override
    {
        const float v = source.load();
        float norm = mode == Level ? juce::jmap(juce::Decibels::gainToDecibels(v, -80.f), -60.f, 0.f, 0.f, 1.f)
                                   : juce::jmap(v, 0.f, 24.f, 0.f, 1.f);
        norm = juce::jlimit(0.f, 1.f, norm);
        shown = norm > shown ? norm : juce::jmax(norm, shown - 0.035f);
        repaint();
    }
    std::atomic<float>& source; Mode mode; bool vert; float shown = 0.f;
};
