#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Fire final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 40};
        core::Param cooling{*this, "cooling", "Остывание", 10, 100, 55};
        core::Param spark{*this, "spark", "Искры", 10, 200, 120};
        core::Param hue{*this, "hue", "Оттенок пламени", 0, 60, 10};

        void begin(core::Frame& f) override
        {
            for (uint16_t i = 0; i < f.count(); ++i) heat_[i] = 0;
        }

        void render(core::Frame& f, uint16_t dtMs) override
        {
            // Simulation runs on its own clock. The lamp renders at a fixed rate,
            // so an effect must never assume "one call == one step" — it advances
            // by the time that actually passed. This is why speed looks the same
            // on the S3 and on the ESP8266.
            const uint16_t stepMs = uint16_t(1000 / (10 + int(speed)));
            accumulator_ += dtMs;
            while (accumulator_ >= stepMs)
            {
                accumulator_ -= stepMs;
                advance(f);
            }
            draw(f);
        }

    private:
        // One column of the matrix is one independent flame.
        void advance(core::Frame& f)
        {
            const uint8_t w = f.width();
            const uint8_t h = f.height();

            for (uint8_t x = 0; x < w; ++x)
            {
                uint8_t* col = &heat_[size_t(x) * h];

                // Cool every cell a little; the taller the matrix, the less each
                // step may take, or the flame never reaches the top.
                for (uint8_t y = 0; y < h; ++y)
                {
                    const uint8_t loss = random8(0, uint8_t((int(cooling) * 10) / h + 2));
                    col[y] = qsub8(col[y], loss);
                }

                // Heat drifts upwards, smeared across the two cells below.
                for (uint8_t y = uint8_t(h - 1); y >= 2; --y)
                    col[y] = uint8_t((col[y - 1] + col[y - 2] + col[y - 2]) / 3);

                // New embers at the base.
                if (random8() < uint8_t(spark))
                    col[random8(2)] = qadd8(col[random8(2)], random8(160, 255));
            }
        }

        void draw(core::Frame& f)
        {
            const uint8_t base = uint8_t(hue);
            for (uint8_t x = 0; x < f.width(); ++x)
                for (uint8_t y = 0; y < f.height(); ++y)
                {
                    const uint8_t t = heat_[(size_t(x) * f.height()) + y];
                    // HeatColor gives the classic black-red-yellow-white ramp;
                    // the hue parameter tints it towards green or violet flame.
                    CRGB c = HeatColor(t);
                    if (base != 0) c = blend(c, CHSV(base, 255, t), uint8_t(base * 4));
                    f.at(x, y) = c;
                }
        }

        uint8_t heat_[MATRIX_WIDTH * MATRIX_HEIGHT] = {};
        uint16_t accumulator_ = 0;
    };
}

REGISTER_EFFECT(Fire, "Огонь", ::core::Tag::Ambient)
