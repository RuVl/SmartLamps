// Snowfall: white flakes appear at the top and drift straight down.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Snow final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 50};
        core::Param density{*this, "density", "Плотность", 1, 100, 30};

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

            // Everything falls one row; the picture is the state.
            for (uint8_t x = 0; x < w; ++x)
                for (uint8_t y = 0; y < top; ++y)
                    f.at(x, y) = f.at(x, uint8_t(y + 1));

            for (uint8_t x = 0; x < w; ++x)
            {
                // No flake directly under another, so they stay single dots.
                const bool free = !f.at(x, uint8_t(top - 1));
                if (free && random16(1500) < uint16_t(density))
                {
                    // Four shades of cold white, so the fall has depth.
                    const uint8_t dim = uint8_t(0x10 * random8(4));
                    f.at(x, top) = CRGB(uint8_t(0xE0 - dim), uint8_t(0xFF - dim), uint8_t(0xFF - dim));
                }
                else
                    f.at(x, top) = CRGB::Black;
            }
        }

        uint16_t accumulator_ = 0;
    };
}

REGISTER_EFFECT(Snow, "Снегопад", ::core::Tag::Ambient)
