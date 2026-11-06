#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UIComponents.h"
#include "PressView.h"

class TwoFarEditor : public juce::AudioProcessorEditor
{
public:
    explicit TwoFarEditor(TwoFarProcessor&);
    ~TwoFarEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    TwoFarProcessor& proc;
    MetalLnF lnf;
    PressView press { proc };
    Meter inMeter { proc.inLevel, Meter::Level, true }, outMeter { proc.outLevel, Meter::Level, true },
          grMeter { proc.grDb, Meter::GainReduction, false };

    // CRUSH
    Knob kInt  { proc.apvts, Param::crushIntensity, "INTENSITY", Pal::orange };
    Knob kSpd  { proc.apvts, Param::crushSpeed, "SPEED", Pal::orange };
    Knob kReps { proc.apvts, Param::crushReps, "REPEATS", Pal::orange };
    Knob kSens { proc.apvts, Param::crushSens, "AUTO SENS", Pal::orange };
    LedButton btnAuto { proc.apvts, Param::crushAuto, "AUTO (TRANSIENT)", Pal::orange };
    juce::TextButton btnPress { "PRESS!" };
    // COMPRESSOR
    LedButton compOn { proc.apvts, Param::compOn, "ON", Pal::green };
    Knob kThr { proc.apvts, Param::compThresh, "THRESH dB", Pal::cyan }, kRat { proc.apvts, Param::compRatio, "RATIO", Pal::cyan },
         kAtk { proc.apvts, Param::compAttack, "ATTACK ms", Pal::cyan }, kRel { proc.apvts, Param::compRelease, "RELEASE ms", Pal::cyan },
         kMk  { proc.apvts, Param::compMakeup, "MAKEUP dB", Pal::cyan }, kCMix { proc.apvts, Param::compMix, "DRY/WET", Pal::cyan };
    // FLANGER
    LedButton flOn { proc.apvts, Param::flOn, "ON", Pal::green }, flSync { proc.apvts, Param::flSync, "SYNC", Pal::yellow };
    Choice flDiv { proc.apvts, Param::flDiv, { "1 bar", "1/2", "1/4", "1/8", "1/16" } };
    Knob kRate { proc.apvts, Param::flRate, "RATE Hz", Pal::purple }, kDep { proc.apvts, Param::flDepth, "DEPTH", Pal::purple },
         kFb { proc.apvts, Param::flFb, "FEEDBACK", Pal::purple }, kDel { proc.apvts, Param::flDelay, "DELAY ms", Pal::purple },
         kFMix { proc.apvts, Param::flMix, "MIX", Pal::purple };
    // DISTORTION
    LedButton dsOn { proc.apvts, Param::dsOn, "ON", Pal::green };
    Choice dsType { proc.apvts, Param::dsType, { "Soft", "Tube", "Hard", "Fold" } };
    Knob kDrv { proc.apvts, Param::dsDrive, "DRIVE dB", Pal::red }, kTone { proc.apvts, Param::dsTone, "TONE Hz", Pal::red },
         kOut { proc.apvts, Param::dsOut, "OUTPUT dB", Pal::red }, kDMix { proc.apvts, Param::dsMix, "MIX", Pal::red };

    juce::Rectangle<int> crushArea, compArea, flArea, dsArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TwoFarEditor)
};
