#include "PluginEditor.h"

static void grid(juce::Rectangle<int> a, std::initializer_list<juce::Component*> items, int cols)
{
    const int rows = ((int) items.size() + cols - 1) / cols, cw = a.getWidth() / cols, ch = a.getHeight() / rows;
    int i = 0;
    for (auto* c : items) { c->setBounds(a.getX() + (i % cols) * cw, a.getY() + (i / cols) * ch, cw, ch); ++i; }
}

TwoFarEditor::TwoFarEditor(TwoFarProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lnf);
    for (juce::Component* c : std::initializer_list<juce::Component*>{ &press, &inMeter, &outMeter, &grMeter,
            &kInt, &kSpd, &kReps, &kSens, &btnAuto, &btnPress, &compOn, &kThr, &kRat, &kAtk, &kRel, &kMk, &kCMix,
            &flOn, &flSync, &flDiv, &kRate, &kDep, &kFb, &kDel, &kFMix, &dsOn, &dsType, &kDrv, &kTone, &kOut, &kDMix })
        addAndMakeVisible(c);

    btnPress.setColour(juce::TextButton::buttonColourId, Pal::red);
    btnPress.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    btnPress.onClick = [this]
    {
        if (auto* prm = proc.apvts.getParameter(Param::crushTrigger))
        {   // cambiar el valor del parámetro dispara la prensa (también automatizable)
            prm->beginChangeGesture(); prm->setValueNotifyingHost(1.f - prm->getValue()); prm->endChangeGesture();
        }
    };
    setSize(1060, 712);
}

TwoFarEditor::~TwoFarEditor() { setLookAndFeel(nullptr); }

static void panel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour accent)
{
    using namespace juce;
    auto f = r.toFloat();
    g.setGradientFill(ColourGradient(Colour(0xff24272c), f.getX(), f.getY(), Colour(0xff141518), f.getX(), f.getBottom(), false));
    g.fillRoundedRectangle(f, 8.f);
    g.setColour(Pal::edge); g.drawRoundedRectangle(f.reduced(0.5f), 8.f, 1.f);
    g.setColour(accent); g.fillRect(f.getX() + 12.f, f.getY() + 10.f, 4.f, 17.f);
    g.setColour(Pal::text); g.setFont(F(15.f, Font::bold));
    g.drawText(title, r.getX() + 22, r.getY() + 8, 180, 20, Justification::centredLeft);
    for (auto p : { Point<float>(f.getRight() - 9, f.getY() + 9), Point<float>(f.getRight() - 9, f.getBottom() - 9),
                    Point<float>(f.getX() + 9, f.getBottom() - 9) })
    { g.setColour(Colour(0xff0a0a0b)); g.fillEllipse(p.x - 4, p.y - 4, 8, 8); g.setColour(Colour(0xff80858d)); g.fillEllipse(p.x - 3, p.y - 3, 6, 6); }
}

void TwoFarEditor::paint(juce::Graphics& g)
{
    using namespace juce;
    g.setGradientFill(ColourGradient(Colour(0xff17181b), 0, 0, Pal::bg, 0, (float) getHeight(), false));
    g.fillAll();
    g.setColour(Colour(0xff0a0a0b)); g.fillRect(0, 0, getWidth(), 56);
    g.setColour(Pal::orange); g.fillRect(0, 56, getWidth(), 3);
    g.setFont(F(34.f, Font::bold));
    g.setColour(Colours::black); g.drawText("2FAR SOUND", 22, 6, 400, 44, Justification::centredLeft);
    g.setColour(Pal::orange);    g.drawText("2FAR SOUND", 20, 4, 400, 44, Justification::centredLeft);
    g.setColour(Pal::dim); g.setFont(F(12.f, Font::bold));
    g.drawText("HYDRAULIC AUDIO CRUSHER  //  CRUSH  .  COMPRESSOR  .  FLANGER  .  DISTORTION", 300, 4, 740, 44, Justification::centredRight);

    panel(g, crushArea, "CRUSH", Pal::orange);
    panel(g, compArea, "COMPRESSOR", Pal::cyan);
    panel(g, flArea, "FLANGER", Pal::purple);
    panel(g, dsArea, "DISTORTION", Pal::red);

    g.setColour(Pal::dim); g.setFont(F(10.f, Font::bold));
    g.drawText("IN", inMeter.getX() - 6, inMeter.getBottom() + 2, 30, 14, Justification::centred);
    g.drawText("OUT", outMeter.getX() - 6, outMeter.getBottom() + 2, 30, 14, Justification::centred);
    g.drawText("GR", grMeter.getX() - 26, grMeter.getY(), 24, grMeter.getHeight(), Justification::centredRight);
}

void TwoFarEditor::resized()
{
    crushArea = { 12, 66, 300, 270 };   compArea = { 12, 344, 300, 358 };
    flArea    = { 748, 66, 300, 300 };  dsArea   = { 748, 374, 300, 328 };

    inMeter.setBounds(318, 70, 18, 600);  outMeter.setBounds(724, 70, 18, 600);
    press.setBounds(340, 66, 380, 640);

    { auto a = crushArea.reduced(10).withTrimmedTop(26);
      auto row = a.removeFromTop(34);
      btnAuto.setBounds(row.removeFromLeft(160)); btnPress.setBounds(row.reduced(2));
      grid(a, { &kInt, &kSpd, &kReps, &kSens }, 2); }

    { auto a = compArea.reduced(10).withTrimmedTop(26);
      compOn.setBounds(a.removeFromTop(26).removeFromLeft(80));
      auto gr = a.removeFromBottom(24); grMeter.setBounds(gr.withTrimmedLeft(30).reduced(0, 3));
      grid(a, { &kThr, &kRat, &kAtk, &kRel, &kMk, &kCMix }, 3); }

    { auto a = flArea.reduced(10).withTrimmedTop(26);
      auto row = a.removeFromTop(28);
      flOn.setBounds(row.removeFromLeft(70)); flSync.setBounds(row.removeFromLeft(80)); flDiv.setBounds(row.reduced(2));
      grid(a, { &kRate, &kDep, &kFb, &kDel, &kFMix }, 3); }

    { auto a = dsArea.reduced(10).withTrimmedTop(26);
      auto row = a.removeFromTop(28);
      dsOn.setBounds(row.removeFromLeft(70)); dsType.setBounds(row.reduced(2));
      grid(a, { &kDrv, &kTone, &kOut, &kDMix }, 2); }
}
