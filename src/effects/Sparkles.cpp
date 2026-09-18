// Confetti: random pixels light up in random hues and fade away.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Sparkles final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 60};
        core::Param density{*this, "density", "Плотность", 1, 30, 8};
        core::Param fade{*this, "fade", "Затухание", 5, 100, 40};

        void render(core::Frame& f, uint16_t dtMs) override
        {
            const uint16_t stepMs = uint16_t(1000 / (10 + int(speed)));
            accumulator_ += dtMs;
            while (accumulator_ >= stepMs)
            {
                accumulator_ -= stepMs;
                f.fade(uint8_t(fade));
                for (int i = 0; i < int(density); ++i)
                {
                    CRGB& px = f.at(random8(f.width()), random8(f.height()));
                    // Only dark cells get a new spark, so sparks read as dots,
                    // not as a wash of colour.
                    if (!px) px = CHSV(random8(), 255, 255);
                }
            }
        }

    private:
        uint16_t accumulator_ = 0;
    };
}

REGISTER_EFFECT(Sparkles, "Конфетти", ::core::Tag::Dynamic)
