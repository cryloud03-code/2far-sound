#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

//==============================================================================
// COMPRESSOR: detector de pico enlazado (stereo link), rodilla suave de 6 dB,
// suavizado attack/release sobre la reducción de ganancia, makeup y dry/wet.
class Compressor
{
public:
    void prepare(double sampleRate) { sr = (float) sampleRate; grSmooth = 0.f; }

    // devuelve la máxima reducción de ganancia del bloque (dB, positivo)
    float process(juce::AudioBuffer<float>& b, float thr, float ratio, float atkMs,
                  float relMs, float makeupDb, float mix)
    {
        const int n = b.getNumSamples(), nc = juce::jmin(b.getNumChannels(), 2);
        const float atk = std::exp(-1.f / (0.001f * atkMs * sr));
        const float rel = std::exp(-1.f / (0.001f * relMs * sr));
        const float makeup = juce::Decibels::decibelsToGain(makeupDb);
        const float knee = 6.f;
        float maxGr = 0.f;

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.f;
            for (int ch = 0; ch < nc; ++ch) peak = juce::jmax(peak, std::abs(b.getReadPointer(ch)[i]));
            const float lvl = juce::Decibels::gainToDecibels(peak, -100.f);
            const float over = lvl - thr;
            float outDb;
            if (2.f * over < -knee)            outDb = lvl;
            else if (2.f * std::abs(over) <= knee)
                outDb = lvl + (1.f / ratio - 1.f) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.f * knee);
            else                               outDb = thr + over / ratio;

            const float target = juce::jmax(0.f, lvl - outDb);
            const float c = target > grSmooth ? atk : rel;
            grSmooth = c * grSmooth + (1.f - c) * target;
            const float g = juce::Decibels::decibelsToGain(-grSmooth) * makeup;

            for (int ch = 0; ch < nc; ++ch)
            {
                float* d = b.getWritePointer(ch);
                const float dry = d[i];
                d[i] = dry * (1.f - mix) + dry * g * mix;
            }
            maxGr = juce::jmax(maxGr, grSmooth);
        }
        return maxGr;
    }
    void reset() { grSmooth = 0.f; }
private:
    float sr = 44100.f, grSmooth = 0.f;
};

//==============================================================================
// FLANGER: línea de retardo con interpolación lineal, LFO senoidal (canal R
// desfasado 90°), feedback positivo/negativo, delay central manual.
class Flanger
{
public:
    void prepare(double sampleRate)
    {
        sr = (float) sampleRate;
        size = (int) (sr * 0.03f) + 4;
        for (auto& v : buf) v.assign((size_t) size, 0.f);
        writePos = 0; phase = 0.f;
    }
    void process(juce::AudioBuffer<float>& b, float rate, float depth, float fb, float delayMs, float mix)
    {
        const int n = b.getNumSamples(), nc = juce::jmin(b.getNumChannels(), 2);
        fb = juce::jlimit(-0.95f, 0.95f, fb);
        for (int i = 0; i < n; ++i)
        {
            for (int ch = 0; ch < nc; ++ch)
            {
                const float lfo = std::sin(juce::MathConstants<float>::twoPi * (phase + (ch == 1 ? 0.25f : 0.f)));
                const float dMs = juce::jlimit(0.05f, 20.f, delayMs + lfo * depth * 4.f);
                float rp = (float) writePos - dMs * 0.001f * sr;
                while (rp < 0.f) rp += (float) size;
                const int i0 = (int) rp % size;
                const int i1 = (i0 + 1) % size;
                const float fr = rp - std::floor(rp);
                const float del = buf[(size_t) ch][(size_t) i0] * (1.f - fr) + buf[(size_t) ch][(size_t) i1] * fr;

                float* d = b.getWritePointer(ch);
                const float in = d[i];
                buf[(size_t) ch][(size_t) writePos] = in + fb * del;
                d[i] = in * (1.f - mix) + (in + del) * 0.7f * mix;
            }
            writePos = (writePos + 1) % size;
            phase += rate / sr;
            if (phase >= 1.f) phase -= 1.f;
        }
    }
private:
    std::array<std::vector<float>, 2> buf;
    int size = 1, writePos = 0;
    float sr = 44100.f, phase = 0.f;
};

//==============================================================================
// DISTORSIÓN: drive previo, 4 curvas (Soft/Tube/Hard/Fold), bloqueo DC,
// filtro Tone (paso bajo 1 polo), salida y mezcla. Sin oversampling (v1).
class Distortion
{
public:
    void prepare(double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset() { lp.fill(0.f); x1.fill(0.f); y1.fill(0.f); }

    void process(juce::AudioBuffer<float>& b, float driveDb, float toneHz, float outDb, float mix, int type)
    {
        const int n = b.getNumSamples(), nc = juce::jmin(b.getNumChannels(), 2);
        const float pre = juce::Decibels::decibelsToGain(driveDb);
        const float out = juce::Decibels::decibelsToGain(outDb);
        const float a = 1.f - std::exp(-juce::MathConstants<float>::twoPi * juce::jmin(toneHz, sr * 0.45f) / sr);
        for (int ch = 0; ch < nc; ++ch)
        {
            float* d = b.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
            {
                const float dry = d[i];
                float y = shape(dry * pre, type);
                const float dc = y - x1[(size_t) ch] + 0.995f * y1[(size_t) ch];
                x1[(size_t) ch] = y; y1[(size_t) ch] = dc; y = dc;
                lp[(size_t) ch] += a * (y - lp[(size_t) ch]);
                d[i] = dry * (1.f - mix) + lp[(size_t) ch] * out * mix;
            }
        }
    }
private:
    static float shape(float x, int type)
    {
        switch (type)
        {
            case 0:  return std::tanh(x);
            case 1:  return x >= 0.f ? 1.f - std::exp(-x) : -0.8f * (1.f - std::exp(1.3f * x));
            case 2:  return juce::jlimit(-0.7f, 0.7f, x) / 0.7f;
            default: return std::sin(x);
        }
    }
    float sr = 44100.f;
    std::array<float, 2> lp{}, x1{}, y1{};
};

//==============================================================================
// Bit/sample-rate reducer cuya cantidad la controla la presión de la prensa.
class BitCrusher
{
public:
    void reset() { held.fill(0.f); counter = 0; }
    void process(juce::AudioBuffer<float>& b, float amt)
    {
        if (amt < 0.002f) { counter = 0; return; }
        const int n = b.getNumSamples(), nc = juce::jmin(b.getNumChannels(), 2);
        const float levels = std::pow(2.f, (16.f - 12.f * amt) - 1.f);
        const int hold = 1 + (int) (amt * amt * 10.f);
        const float mix = juce::jmin(1.f, amt * 1.6f);
        for (int i = 0; i < n; ++i)
        {
            if (counter == 0)
                for (int ch = 0; ch < nc; ++ch)
                    held[(size_t) ch] = std::round(b.getReadPointer(ch)[i] * levels) / levels;
            counter = (counter + 1) % hold;
            for (int ch = 0; ch < nc; ++ch)
            {
                float* d = b.getWritePointer(ch);
                d[i] = d[i] * (1.f - mix) + held[(size_t) ch] * mix;
            }
        }
    }
private:
    std::array<float, 2> held{};
    int counter = 0;
};

//==============================================================================
// Máquina de estados de la prensa: Down -> Hold (impacto) -> Up, con repeticiones.
class CrushEngine
{
public:
    enum class Phase { Idle, Down, Hold, Up };
    void reset() { phase = Phase::Idle; t = 0.f; repsLeft = 0; }
    bool isActive() const { return phase != Phase::Idle; }
    void trigger(int reps) { if (phase == Phase::Idle) { repsLeft = juce::jmax(1, reps); phase = Phase::Down; t = 0.f; } }

    // devuelve presión 0..1 (ya multiplicada por intensidad). impact=true al tocar fondo.
    float advance(int numSamples, double sr, float speed, float intensity, bool& impact)
    {
        impact = false;
        if (phase == Phase::Idle) return 0.f;
        const float cycle = 1.6f / juce::jmax(0.1f, speed);
        const float dt = (float) numSamples / (float) sr;
        const float dur = phase == Phase::Down ? 0.34f : phase == Phase::Hold ? 0.16f : 0.50f;
        t += dt / (cycle * dur);
        if (t >= 1.f)
        {
            t = 0.f;
            switch (phase)
            {
                case Phase::Down: phase = Phase::Hold; impact = true; break;
                case Phase::Hold: phase = Phase::Up; break;
                case Phase::Up:   phase = (--repsLeft > 0) ? Phase::Down : Phase::Idle; break;
                default: break;
            }
        }
        float p = 0.f;
        switch (phase)
        {
            case Phase::Down: p = t * t; break;
            case Phase::Hold: p = 1.f; break;
            case Phase::Up:   p = 1.f - t * t * (3.f - 2.f * t); break;
            default: break;
        }
        return p * intensity;
    }
private:
    Phase phase = Phase::Idle;
    float t = 0.f;
    int repsLeft = 0;
};
