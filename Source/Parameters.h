#pragma once
#include <JuceHeader.h>

namespace Param
{
    // CRUSH
    inline constexpr const char* crushIntensity = "crush_intensity";
    inline constexpr const char* crushSpeed     = "crush_speed";
    inline constexpr const char* crushReps      = "crush_reps";
    inline constexpr const char* crushAuto      = "crush_auto";
    inline constexpr const char* crushSens      = "crush_sens";
    inline constexpr const char* crushTrigger   = "crush_trigger";
    // COMPRESSOR
    inline constexpr const char* compOn      = "comp_on";
    inline constexpr const char* compThresh  = "comp_thresh";
    inline constexpr const char* compRatio   = "comp_ratio";
    inline constexpr const char* compAttack  = "comp_attack";
    inline constexpr const char* compRelease = "comp_release";
    inline constexpr const char* compMakeup  = "comp_makeup";
    inline constexpr const char* compMix     = "comp_mix";
    // FLANGER
    inline constexpr const char* flOn    = "fl_on";
    inline constexpr const char* flRate  = "fl_rate";
    inline constexpr const char* flDepth = "fl_depth";
    inline constexpr const char* flFb    = "fl_fb";
    inline constexpr const char* flDelay = "fl_delay";
    inline constexpr const char* flMix   = "fl_mix";
    inline constexpr const char* flSync  = "fl_sync";
    inline constexpr const char* flDiv   = "fl_div";
    // DISTORTION
    inline constexpr const char* dsOn    = "ds_on";
    inline constexpr const char* dsDrive = "ds_drive";
    inline constexpr const char* dsTone  = "ds_tone";
    inline constexpr const char* dsOut   = "ds_out";
    inline constexpr const char* dsMix   = "ds_mix";
    inline constexpr const char* dsType  = "ds_type";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto F = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 1.f)
    {
        p.push_back(std::make_unique<AudioParameterFloat>(ParameterID{ id, 1 }, name,
                    NormalisableRange<float>(lo, hi, 0.001f, skew), def));
    };
    auto B = [&](const char* id, const char* name, bool def)
    {
        p.push_back(std::make_unique<AudioParameterBool>(ParameterID{ id, 1 }, name, def));
    };
    auto C = [&](const char* id, const char* name, StringArray items, int def)
    {
        p.push_back(std::make_unique<AudioParameterChoice>(ParameterID{ id, 1 }, name, items, def));
    };

    // CRUSH
    F(Param::crushIntensity, "Crush Intensity", 0.1f, 1.f, 0.85f);
    F(Param::crushSpeed,     "Crush Speed",     0.25f, 4.f, 1.f, 0.5f);
    p.push_back(std::make_unique<AudioParameterInt>(ParameterID{ Param::crushReps, 1 }, "Crush Repeats", 1, 8, 1));
    B(Param::crushAuto,    "Crush Auto (Transient)", false);
    F(Param::crushSens,    "Crush Sensitivity", 0.f, 1.f, 0.5f);
    B(Param::crushTrigger, "Crush Trigger", false);

    // COMPRESSOR
    B(Param::compOn, "Comp On", true);
    F(Param::compThresh,  "Comp Threshold", -60.f, 0.f, -18.f);
    F(Param::compRatio,   "Comp Ratio", 1.f, 20.f, 4.f, 0.5f);
    F(Param::compAttack,  "Comp Attack", 0.1f, 100.f, 10.f, 0.4f);
    F(Param::compRelease, "Comp Release", 10.f, 1000.f, 120.f, 0.4f);
    F(Param::compMakeup,  "Comp Makeup", 0.f, 24.f, 0.f);
    F(Param::compMix,     "Comp Mix", 0.f, 1.f, 1.f);

    // FLANGER
    B(Param::flOn, "Flanger On", false);
    F(Param::flRate,  "Flanger Rate", 0.05f, 10.f, 0.4f, 0.4f);
    F(Param::flDepth, "Flanger Depth", 0.f, 1.f, 0.6f);
    F(Param::flFb,    "Flanger Feedback", -0.95f, 0.95f, 0.4f);
    F(Param::flDelay, "Flanger Center Delay", 0.1f, 10.f, 2.f, 0.5f);
    F(Param::flMix,   "Flanger Mix", 0.f, 1.f, 0.5f);
    B(Param::flSync,  "Flanger Tempo Sync", false);
    C(Param::flDiv,   "Flanger Sync Division", { "1 bar", "1/2", "1/4", "1/8", "1/16" }, 0);

    // DISTORTION
    B(Param::dsOn, "Distortion On", false);
    F(Param::dsDrive, "Distortion Drive", 0.f, 40.f, 12.f);
    F(Param::dsTone,  "Distortion Tone", 200.f, 16000.f, 6000.f, 0.3f);
    F(Param::dsOut,   "Distortion Output", -24.f, 6.f, -6.f);
    F(Param::dsMix,   "Distortion Mix", 0.f, 1.f, 1.f);
    C(Param::dsType,  "Distortion Type", { "Soft", "Tube", "Hard", "Fold" }, 0);

    return { p.begin(), p.end() };
}
