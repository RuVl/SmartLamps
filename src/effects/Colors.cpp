// The whole matrix in one colour, slowly walking around the hue wheel.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Colors final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 20};
        core::Param saturation{*this, "saturation", "Насыщенность", 0, 100, 100};

        void render(core::Frame& f, uint16_t dtMs) override
        {
            // A quarter of Rainbow's pace: speed 100 cycles in ~2.6 s, speed 10 in ~26 s.
            hue16_ = uint16_t(hue16_ + (dtMs * uint16_t(speed)) / 4);
            const uint8_t sat = uint8_t((int(saturation) * 255) / 100);
            f.fill(CHSV(uint8_t(hue16_ >> 8), sat, 255));
        }

    private:
        uint16_t hue16_ = 0;
    };
}

REGISTER_EFFECT(Colors, "Смена цвета", ::core::Tag::Ambient)
