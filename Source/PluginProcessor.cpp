#include "PluginProcessor.h"
#include "PluginEditor.h"

TwoFarProcessor::TwoFarProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
}

bool TwoFarProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void TwoFarProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    comp.prepare(sr); flanger.prepare(sr); dist.prepare(sr); crusher.reset(); engine.reset();
    envFast = envSlow = 0.f; lastTrig = -1.f; lockout = 0;
}

void TwoFarProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, n);

    auto P = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    const int nc = juce::jmin(buffer.getNumChannels(), 2);

    inLevel.store(buffer.getMagnitude(0, n));

    // ---------- disparo de la prensa: manual (cambio de parámetro) o por transientes
    const float trig = P(Param::crushTrigger);
    const int reps = (int) std::round(P(Param::crushReps));
    if (lastTrig < 0.f) lastTrig = trig;
    else if (trig != lastTrig) { lastTrig = trig; engine.trigger(reps); }

    if (P(Param::crushAuto) > 0.5f)
    {
        const float fa = 1.f - std::exp(-1.f / (0.001f * (float) sr)), fr = 1.f - std::exp(-1.f / (0.02f * (float) sr));
        const float sa = 1.f - std::exp(-1.f / (0.05f * (float) sr)), srl = 1.f - std::exp(-1.f / (0.2f * (float) sr));
        const float ratio = 3.f - 1.8f * P(Param::crushSens);
        for (int i = 0; i < n; ++i)
        {
            float x = 0.f;
            for (int ch = 0; ch < nc; ++ch) x = juce::jmax(x, std::abs(buffer.getReadPointer(ch)[i]));
            envFast += (x > envFast ? fa : fr) * (x - envFast);
            envSlow += (x > envSlow ? sa : srl) * (x - envSlow);
            if (lockout > 0) --lockout;
            else if (envFast > 0.04f && envFast > envSlow * ratio)
            {
                engine.trigger(reps);
                lockout = (int) (0.15 * sr);
            }
        }
    }

    // ---------- cadena: Distortion -> Flanger -> Compressor -> Crush
    if (P(Param::dsOn) > 0.5f)
        dist.process(buffer, P(Param::dsDrive), P(Param::dsTone), P(Param::dsOut), P(Param::dsMix),
                     (int) std::round(P(Param::dsType)));

    if (P(Param::flOn) > 0.5f)
    {
        float rate = P(Param::flRate);
        if (P(Param::flSync) > 0.5f)
        {
            double bpm = 120.0;
            if (auto* ph = getPlayHead())
                if (auto pos = ph->getPosition())
                    if (auto b = pos->getBpm()) bpm = *b;
            static const float beats[] = { 4.f, 2.f, 1.f, 0.5f, 0.25f };
            rate = (float) (bpm / 60.0) / beats[juce::jlimit(0, 4, (int) std::round(P(Param::flDiv)))];
        }
        flanger.process(buffer, rate, P(Param::flDepth), P(Param::flFb), P(Param::flDelay), P(Param::flMix));
    }

    float gr = 0.f;
    if (P(Param::compOn) > 0.5f)
        gr = comp.process(buffer, P(Param::compThresh), P(Param::compRatio), P(Param::compAttack),
                          P(Param::compRelease), P(Param::compMakeup), P(Param::compMix));
    grDb.store(gr);

    bool impact = false;
    const float p = engine.advance(n, sr, P(Param::crushSpeed), P(Param::crushIntensity), impact);
    pressure.store(p);
    if (impact) impactCount.fetch_add(1);
    crusher.process(buffer, p);

    outLevel.store(buffer.getMagnitude(0, n));
}

juce::AudioProcessorEditor* TwoFarProcessor::createEditor() { return new TwoFarEditor(*this); }

void TwoFarProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void TwoFarProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TwoFarProcessor(); }
