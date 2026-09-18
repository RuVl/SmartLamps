// The Matrix: green drops with fading trails running down the columns.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Matrix final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 50};
        core::Param density{*this, "density", "Плотность", 1, 100, 20};

        void render(core::Frame& f, uint16_t dtMs) override
        {
            const uint16_t stepMs = uint16_t(1000 / (5 + int(speed)));
            accumulator_ += dtMs;
            while (accumulator_ >= stepMs)
            {
                accumulator_ -= stepMs;
                advance(f);
            }
        }

    private:
        void advance(core::Frame& f)
        {
            const uint8_t w = f.width();
            const uint8_t top = uint8_t(f.height() - 1);

            // The top row is the source: a drop is born bright and dims by a
            // fixed amount each step while the column below carries its trail.
            for (uint8_t x = 0; x < w; ++x)
            {
                CRGB& head = f.at(x, top);
                if (!head)
                {
                    if (random16(2000) < uint16_t(density)) head = CRGB(0, 255, 0);
                }
                else
                {
                    head.g = qsub8(head.g, 0x20);
                }
            }

            for (uint8_t x = 0; x < w; ++x)
                for (uint8_t y = 0; y < top; ++y)
                    f.at(x, y) = f.at(x, uint8_t(y + 1));
        }

        uint16_t accumulator_ = 0;
    };
}

REGISTER_EFFECT(Matrix, "Матрица", ::core::Tag::Dynamic)
