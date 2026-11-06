#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "Parameters.h"
#include "DSP.h"

class TwoFarProcessor : public juce::AudioProcessor
{
public:
    TwoFarProcessor();
    ~TwoFarProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "2far sound"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // datos para la GUI (se escriben desde el hilo de audio, se leen desde la GUI)
    std::atomic<float> inLevel { 0.f }, outLevel { 0.f }, grDb { 0.f }, pressure { 0.f };
    std::atomic<int>   impactCount { 0 };

private:
    Compressor comp; Flanger flanger; Distortion dist; BitCrusher crusher; CrushEngine engine;
    double sr = 44100.0;
    float envFast = 0.f, envSlow = 0.f, lastTrig = -1.f;
    int lockout = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TwoFarProcessor)
};
