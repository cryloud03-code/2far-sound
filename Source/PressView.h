#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UIComponents.h"

// Animación principal: prensa hidráulica + personaje dibujado proceduralmente
// (vectores, NO una imagen estática). La cara se deforma según la altura real
// disponible bajo la placa (resorte con rebote), con ojos/mejillas/boca/gafas
// reaccionando, salpicaduras, onomatopeya, estrellas mareadas y sacudida.
class PressView : public juce::Component, private juce::Timer
{
public:
    explicit PressView(TwoFarProcessor& p) : proc(p) { startTimerHz(60); }
    ~PressView() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        const auto G = geo();
        g.addTransform(AffineTransform::translation(shakeX, shakeY));
        paintBackdrop(g, G);
        paintFrame(g, G);
        paintCharacter(g, G);
        paintPlaten(g, G);
        paintParticles(g);
        paintSplat(g, G);
        paintLcd(g, G);
        if (flash > 0.01f) { g.setColour(Colours::white.withAlpha(flash * 0.35f)); g.fillAll(); }
    }

private:
    static constexpr float H0 = 286.f;
    struct Geo { float w, h, cx, floorY, restBottom, platenBottom, charH; };
    struct Particle { float x, y, vx, vy, life, size, rot; juce::Colour c; bool star; };

    Geo geo() const
    {
        Geo G; G.w = (float) getWidth(); G.h = (float) getHeight(); G.cx = G.w * 0.5f;
        G.floorY = G.h - 84.f; G.restBottom = G.floorY - H0 - 8.f;
        G.platenBottom = G.restBottom + platenP * (G.floorY - 36.f - G.restBottom);
        G.charH = G.floorY - G.platenBottom;
        return G;
    }

    void timerCallback() override
    {
        const float target = proc.pressure.load();
        platenP += (target - platenP) * 0.55f;
        const auto G = geo();
        // la altura de la cara sigue a la placa; al subir recupera con resorte y rebota
        const float tgtH = juce::jmin(H0, G.charH);
        fv += (tgtH - fh) * 0.26f; fv *= 0.80f; fh += fv;
        if (fh > G.charH) { fh = G.charH; fv = juce::jmin(fv, 0.f); }
        fh = juce::jmax(fh, 30.f);

        const int ic = proc.impactCount.load();
        if (ic != lastImpact)
        {
            lastImpact = ic; spawnSplash(G); shake = 1.f; flash = 1.f; splatT = 1.f; dizzy = 1.f;
            static const char* words[] = { "SPLAT!", "CRUNCH!", "BONK!", "SQUISH!" };
            word = words[rng.nextInt(4)];
        }
        shake *= 0.90f; flash *= 0.80f; splatT = juce::jmax(0.f, splatT - 0.012f); dizzy = juce::jmax(0.f, dizzy - 0.006f);
        shakeX = (rng.nextFloat() - 0.5f) * 12.f * shake; shakeY = (rng.nextFloat() - 0.5f) * 8.f * shake;
        time += 1.f / 60.f;

        for (auto& p : particles) { p.x += p.vx; p.y += p.vy; p.vy += 0.45f; p.vx *= 0.99f; p.life -= 0.014f; p.rot += 0.15f; }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0.f; }), particles.end());
        repaint();
    }

    void spawnSplash(const Geo& G)
    {
        const juce::Colour cols[] = { juce::Colour(0xfff2b8cc), juce::Colour(0xff7a5cc8), juce::Colours::white,
                                      juce::Colour(0xff15121c), juce::Colour(0xfffde3d3), juce::Colour(0xffffd23f) };
        const float y = G.floorY - 40.f;
        for (int i = 0; i < 56; ++i)
        {
            const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi, sp = 3.f + rng.nextFloat() * 10.f;
            particles.push_back({ G.cx + (rng.nextFloat() - 0.5f) * 80.f, y, std::cos(a) * sp * 1.5f, std::sin(a) * sp - 5.f, 1.f,
                                  3.f + rng.nextFloat() * 9.f, rng.nextFloat() * 6.f, cols[rng.nextInt(6)], i % 7 == 0 });
        }
    }

    //------------------------------------------------------------------ dibujo
    static void metalH(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour mid)
    {
        juce::ColourGradient cg(mid.darker(0.7f), r.getX(), 0.f, mid.darker(0.7f), r.getRight(), 0.f, false);
        cg.addColour(0.25, mid.brighter(0.2f)); cg.addColour(0.45, mid.brighter(0.5f)); cg.addColour(0.75, mid.darker(0.2f));
        g.setGradientFill(cg); g.fillRect(r);
    }
    static void rivet(juce::Graphics& g, float x, float y)
    {
        g.setColour(juce::Colour(0xff0a0a0b)); g.fillEllipse(x - 4.f, y - 4.f, 8.f, 8.f);
        g.setColour(juce::Colour(0xff9ea3ab)); g.fillEllipse(x - 3.f, y - 3.f, 6.f, 6.f);
        g.setColour(juce::Colours::white.withAlpha(0.6f)); g.fillEllipse(x - 1.5f, y - 2.f, 2.f, 2.f);
    }

    void paintBackdrop(juce::Graphics& g, const Geo& G)
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1b1d22), 0, 0, juce::Colour(0xff0a0b0d), 0, G.h, false));
        g.fillAll();
        g.setColour(juce::Colours::white.withAlpha(0.035f));
        for (float x = 0; x < G.w; x += 30.f) g.drawVerticalLine((int) x, 0.f, G.h);
        for (float y = 0; y < G.h; y += 30.f) g.drawHorizontalLine((int) y, 0.f, G.w);
        // sombra bajo el personaje
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillEllipse(G.cx - 120, G.floorY - 8, 240, 20);
    }

    void paintFrame(juce::Graphics& g, const Geo& G)
    {
        using namespace juce;
        const Colour steel(0xff6b7078);
        metalH(g, { 6.f, 8.f, 34.f, G.floorY + 20.f - 8.f }, steel);
        metalH(g, { G.w - 40.f, 8.f, 34.f, G.floorY + 20.f - 8.f }, steel);
        for (float y = 90.f; y < G.floorY; y += 70.f) { rivet(g, 23.f, y); rivet(g, G.w - 23.f, y); }
        // viga superior
        g.setGradientFill(ColourGradient(Colour(0xff8a9099), 0, 8, Colour(0xff2b2e34), 0, 66, false));
        g.fillRect(6.f, 8.f, G.w - 12.f, 58.f);
        for (float x = 24.f; x < G.w - 10.f; x += 44.f) rivet(g, x, 18.f);
        // cilindro hidráulico
        metalH(g, { G.cx - 42.f, 66.f, 84.f, 62.f }, Colour(0xff565b63));
        g.setColour(Colour(0xff15161a)); g.fillRect(G.cx - 42.f, 84.f, 84.f, 4.f); g.fillRect(G.cx - 42.f, 108.f, 84.f, 4.f);
        // base
        g.setGradientFill(ColourGradient(Colour(0xff4a4e55), 0, G.floorY, Colour(0xff1a1b1f), 0, G.floorY + 26, false));
        g.fillRect(6.f, G.floorY, G.w - 12.f, 26.f);
        g.setColour(Colours::black); g.drawHorizontalLine((int) G.floorY, 6.f, G.w - 6.f);
        // manómetro
        const float gx = G.cx + 105.f, gy = 38.f, gr = 21.f;
        g.setColour(Colours::black); g.fillEllipse(gx - gr - 3, gy - gr - 3, 2 * gr + 6, 2 * gr + 6);
        g.setColour(Colour(0xffe9e6da)); g.fillEllipse(gx - gr, gy - gr, 2 * gr, 2 * gr);
        for (int i = 0; i <= 10; ++i)
        {
            const float a = -0.75f * MathConstants<float>::pi + 1.5f * MathConstants<float>::pi * i / 10.f;
            g.setColour(i > 7 ? Pal::red : Colours::black);
            g.drawLine(gx + std::sin(a) * gr * 0.72f, gy - std::cos(a) * gr * 0.72f, gx + std::sin(a) * gr * 0.92f, gy - std::cos(a) * gr * 0.92f, 1.5f);
        }
        const float na = -0.75f * MathConstants<float>::pi + 1.5f * MathConstants<float>::pi * jlimit(0.f, 1.f, platenP);
        g.setColour(Pal::red); g.drawLine(gx, gy, gx + std::sin(na) * gr * 0.8f, gy - std::cos(na) * gr * 0.8f, 2.f);
        // lámpara de aviso
        const float lx = G.cx - 105.f; const bool hot = platenP > 0.03f;
        if (hot) { g.setGradientFill(ColourGradient(Pal::red.withAlpha(0.6f), lx, gy, Pal::red.withAlpha(0.f), lx + 40, gy, true)); g.fillEllipse(lx - 40, gy - 40, 80, 80); }
        g.setColour(Colours::black); g.fillEllipse(lx - 14, gy - 14, 28, 28);
        g.setColour(hot ? Pal::red.brighter(0.4f) : Colour(0xff3a1414)); g.fillEllipse(lx - 11, gy - 11, 22, 22);
    }

    void paintPlaten(juce::Graphics& g, const Geo& G)
    {
        using namespace juce;
        const float top = G.platenBottom - 30.f;
        metalH(g, { G.cx - 15.f, 128.f, 30.f, top - 128.f + 4.f }, Colour(0xffb9bec6));      // vástago cromado
        Rectangle<float> pl(G.cx - 150.f, top, 300.f, 30.f);
        g.setGradientFill(ColourGradient(Colour(0xff9aa0a9), 0, pl.getY(), Colour(0xff2a2d33), 0, pl.getBottom(), false));
        g.fillRoundedRectangle(pl, 4.f);
        { // franjas de peligro
            Graphics::ScopedSaveState s(g); g.reduceClipRegion(pl.withTrimmedTop(20.f).toNearestInt());
            for (float x = pl.getX() - 20.f; x < pl.getRight(); x += 28.f)
            {
                Path st; st.addQuadrilateral(x, pl.getBottom(), x + 14, pl.getBottom(), x + 24, pl.getBottom() - 10, x + 10, pl.getBottom() - 10);
                g.setColour(Pal::yellow); g.fillPath(st);
            }
        }
        g.setColour(Colours::black); g.drawRoundedRectangle(pl, 4.f, 2.f);
        rivet(g, pl.getX() + 14, pl.getY() + 9); rivet(g, pl.getRight() - 14, pl.getY() + 9);
    }

    void paintLcd(juce::Graphics& g, const Geo& G)
    {
        using namespace juce;
        Rectangle<float> r(G.cx - 100.f, G.h - 50.f, 200.f, 40.f);
        g.setColour(Colour(0xff050a06)); g.fillRoundedRectangle(r, 5.f);
        g.setColour(Pal::edge); g.drawRoundedRectangle(r, 5.f, 2.f);
        g.setColour(Pal::green.withAlpha(0.6f)); g.setFont(F(10.f, Font::bold));
        g.drawText("PRESSURE", r.withTrimmedLeft(10.f).withTrimmedTop(4.f), Justification::topLeft);
        g.setColour(Pal::green); g.setFont(Font(FontOptions(Font::getDefaultMonospacedFontName(), 28.f, Font::bold)));
        g.drawText(String::formatted("%03d%%", (int) std::round(jlimit(0.f, 1.f, platenP) * 100.f)), r.withTrimmedRight(10.f), Justification::centredRight);
    }

    void paintParticles(juce::Graphics& g)
    {
        for (const auto& p : particles)
        {
            g.setColour(p.c.withAlpha(juce::jlimit(0.f, 1.f, p.life * 1.5f)));
            if (p.star) { juce::Path st; st.addStar({ p.x, p.y }, 5, p.size * 0.4f, p.size * 1.1f, p.rot); g.fillPath(st); }
            else g.fillEllipse(p.x - p.size * 0.5f, p.y - p.size * 0.5f, p.size, p.size * (0.7f + 0.3f * std::sin(p.rot)));
        }
    }

    void paintSplat(juce::Graphics& g, const Geo& G)
    {
        using namespace juce;
        if (splatT <= 0.f) return;
        const float sc = 1.f + splatT * 0.45f, y = G.floorY - 230.f;
        Graphics::ScopedSaveState s(g);
        g.addTransform(AffineTransform::rotation(-0.14f, G.cx, y));
        g.setFont(F(46.f * sc, Font::bold));
        Rectangle<float> r(G.cx - 180.f, y - 40.f, 360.f, 80.f);
        const float a = jlimit(0.f, 1.f, splatT * 2.f);
        g.setColour(Colours::black.withAlpha(a));
        for (auto o : { Point<float>(-3, -3), Point<float>(3, -3), Point<float>(-3, 3), Point<float>(3, 3) }) g.drawText(word, r.translated(o.x, o.y), Justification::centred);
        g.setColour(Pal::yellow.withAlpha(a)); g.drawText(word, r, Justification::centred);
    }

    //------------------------------------------------------------------ personaje
    static float sstep(float t) { t = juce::jlimit(0.f, 1.f, t); return t * t * (3.f - 2.f * t); }

    void paintCharacter(juce::Graphics& g, const Geo& G)
    {
        using namespace juce;
        const float sy = fh / H0;
        const float sx = sy < 1.f ? jmin(1.9f, 1.f + (1.f - sy) * 1.0f) : jmax(0.8f, 1.f - (sy - 1.f) * 0.7f);
        const float dp = jlimit(0.f, 1.f, (H0 - fh) / (H0 * 0.86f));          // 0 = intacto, 1 = plano
        const float bulge = sstep(dp / 0.8f);
        {
            Graphics::ScopedSaveState ss(g);
            g.addTransform(AffineTransform::scale(sx, sy).translated(G.cx, G.floorY));

            const Colour skin(0xfffde3d3), shade(0xfff1b9a5), hair(0xff15121c), hairHi(0xff3a3350),
                         pink(0xfff2b8cc), eye(0xff7a5cc8);
            // camisa y cuello
            { Path s; s.startNewSubPath(-125, 0); s.lineTo(-100, -62); s.quadraticTo(0, -94, 100, -62); s.lineTo(125, 0); s.closeSubPath();
              g.setColour(pink); g.fillPath(s); g.setColour(pink.darker(0.35f)); g.strokePath(s, PathStrokeType(2.f)); }
            g.setColour(shade); g.fillRect(-18.f, -98.f, 36.f, 36.f);

            // pelo trasero y mechones laterales
            const float hw = 104.f + dp * 16.f;
            g.setColour(hair); g.fillEllipse(-hw, -286.f, hw * 2.f, 218.f);
            for (int sgn : { -1, 1 })
            { Path l; l.startNewSubPath(sgn * hw, -190); l.lineTo(sgn * (hw + 16), -58); l.lineTo(sgn * (hw - 34), -84); l.lineTo(sgn * 70.f, -170); l.closeSubPath(); g.fillPath(l); }

            // cara, mejillas que se hinchan
            const float fr = 78.f + dp * 22.f;
            g.setColour(skin); g.fillEllipse(-fr, -258.f, fr * 2.f, 176.f);
            for (int sgn : { -1, 1 })
            {
                const float cr = 14.f + dp * 34.f;
                g.setColour(skin); g.fillEllipse(sgn * (fr - 8.f) - cr, -138.f - cr * 0.7f, cr * 2.f, cr * 1.4f);
                g.setColour(Colour(0xffff8fa8).withAlpha(0.5f)); g.fillEllipse(sgn * 48.f - (18.f + dp * 16.f), -134.f - (9.f + dp * 8.f), 2.f * (18.f + dp * 16.f), 2.f * (9.f + dp * 8.f));
            }

            // ojos: se hinchan y acaban en X
            for (int sgn : { -1, 1 })
            {
                const float ex = sgn * 34.f, ey = -168.f;
                if (dp < 0.82f)
                {
                    const float rx = 19.f * (1.f + 0.55f * bulge), ry = 21.f * (1.f + 0.7f * bulge);
                    const float jit = std::sin(time * 40.f + sgn) * dp * 2.f;
                    g.setColour(Colours::white); g.fillEllipse(ex - rx, ey - ry, rx * 2, ry * 2);
                    g.setColour(hair); g.drawEllipse(ex - rx, ey - ry, rx * 2, ry * 2, 2.5f);
                    const float ix = ex + jit - sgn * 2.f, ir = 12.f * (1.f + 0.3f * bulge);
                    g.setColour(eye); g.fillEllipse(ix - ir, ey - ir * 1.25f, ir * 2, ir * 2.5f);
                    g.setColour(Colour(0xff1a1030)); const float pr = 5.f + 5.f * bulge; g.fillEllipse(ix - pr, ey - pr, pr * 2, pr * 2);
                    g.setColour(Colours::white); g.fillEllipse(ix - 7, ey - 13, 6, 8);
                }
                else
                {
                    g.setColour(hair);
                    g.drawLine(ex - 14, ey - 14, ex + 14, ey + 14, 5.f); g.drawLine(ex - 14, ey + 14, ex + 14, ey - 14, 5.f);
                }
            }

            // gafas: se tuercen, bajan y se agrietan
            {
                Graphics::ScopedSaveState gs(g);
                g.addTransform(AffineTransform::rotation(dp * 0.32f, 0.f, -168.f).translated(0.f, dp * 16.f));
                const Colour frame(0xff1a2330);
                for (int sgn : { -1, 1 })
                {
                    Rectangle<float> lens(sgn < 0 ? -62.f : 10.f, -192.f, 52.f, 46.f);
                    g.setColour(Colour(0x2890c8ff)); g.fillRoundedRectangle(lens, 12.f);
                    g.setColour(frame); g.drawRoundedRectangle(lens, 12.f, 4.f);
                    g.drawLine(sgn * 62.f, -178.f, sgn * 92.f, -184.f, 3.f);
                    if (dp > 0.85f) { g.setColour(Colours::white.withAlpha(0.8f)); g.drawLine(lens.getX() + 8, lens.getY() + 6, lens.getRight() - 10, lens.getBottom() - 6, 1.5f); g.drawLine(lens.getCentreX(), lens.getY() + 4, lens.getCentreX() - 8, lens.getBottom() - 4, 1.5f); }
                }
                g.setColour(frame); g.drawLine(-10, -172, 10, -172, 3.5f);
            }

            // boca + pocky
            {
                const float mw = 12.f + dp * 36.f, mh = 4.f + std::sin(jmin(dp, 1.f) * MathConstants<float>::pi) * 24.f, mx = 6.f, my = -120.f;
                g.setColour(Colour(0xff4a1020)); g.fillEllipse(mx - mw, my - mh, mw * 2, mh * 2);
                if (dp > 0.7f)
                {
                    const float t = (dp - 0.7f) / 0.3f;
                    g.setColour(Colour(0xffff7fa0)); g.fillRoundedRectangle(mx - 10.f, my, 20.f, 8.f + 22.f * t, 8.f);
                }
                const float x0 = mx + mw * 0.3f, y0 = my - 1.f;
                const float len = dp < 0.6f ? 66.f : 30.f;
                g.setColour(Colour(0xffcf9a5a)); g.drawLine(x0, y0, x0 + len, y0 + len * 0.35f, 5.f);
                g.setColour(Colour(0xff5a3320)); g.drawLine(x0 + len * 0.4f, y0 + len * 0.14f, x0 + len, y0 + len * 0.35f, 5.f);
                if (dp >= 0.6f)
                {   const float k = sstep((dp - 0.6f) / 0.4f);
                    const float bx = x0 + 34.f + k * 40.f, by = y0 + 12.f + k * 46.f;
                    g.setColour(Colour(0xff5a3320)); g.drawLine(bx, by, bx + 30.f, by + 10.f, 5.f); }
            }

            // flequillo recto con puntas, por encima de los ojos
            { Path b; b.startNewSubPath(-hw + 2, -200); b.quadraticTo(-82, -296, 0, -296); b.quadraticTo(82, -296, hw - 2, -200);
              b.lineTo(hw - 14, -120); b.lineTo(66, -190);
              const float xs[] = { 50, 36, 22, 8, -8, -22, -36, -50 };
              for (int k = 0; k < 8; ++k) b.lineTo(xs[k], (k % 2 == 0) ? -178.f : -206.f);
              b.lineTo(-66, -190); b.lineTo(-hw + 14, -120); b.closeSubPath();
              g.setColour(hair); g.fillPath(b);
              g.setColour(hairHi); Path hl; hl.addCentredArc(-30, -250, 40, 26, 0, -1.9f, -0.5f, true); g.strokePath(hl, PathStrokeType(4.f)); }

            // gota de sudor
            if (dp > 0.12f) { g.setColour(Colour(0xcc8fd8ff)); g.fillEllipse(-104.f, -236.f, 12.f + dp * 6.f, 16.f + dp * 8.f); }
        }

        // estrellas mareadas (espacio de pantalla)
        if (dizzy > 0.f)
        {
            const float topY = G.floorY - fh - 12.f;
            for (int i = 0; i < 3; ++i)
            {
                const float a = time * 6.f + i * 2.094f;
                Path st; st.addStar({ G.cx + std::cos(a) * 70.f * sx * 0.8f, topY + std::sin(a) * 12.f }, 5, 5.f, 11.f, a);
                g.setColour(Pal::yellow.withAlpha(jmin(1.f, dizzy * 2.f))); g.fillPath(st);
            }
        }
    }

    TwoFarProcessor& proc;
    float platenP = 0.f, fh = H0, fv = 0.f, shake = 0.f, shakeX = 0.f, shakeY = 0.f, flash = 0.f, splatT = 0.f, dizzy = 0.f, time = 0.f;
    int lastImpact = 0;
    juce::String word = "SPLAT!";
    std::vector<Particle> particles;
    juce::Random rng;
};
