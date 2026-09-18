// A square bouncing off the edges, changing colour on every bounce.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Cube final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 40};
        core::Param size{*this, "size", "Размер", 1, 5, 2};

        void begin(core::Frame& f) override
        {
            x_ = int16_t((f.width() / 2) << 8);
            y_ = int16_t((f.height() / 2) << 8);
            vx_ = int16_t(random8(8, 20));
            vy_ = int16_t(random8(8, 20));
            recolour();
        }

        void render(core::Frame& f, uint16_t dtMs) override
        {
            const uint8_t n = uint8_t(size);
            // Velocity is in 1/256 px per 16 ms at speed 25; the setting scales it.
            const int gain = int(speed);
            x_ = int16_t(x_ + (vx_ * gain * dtMs) / 400);
            y_ = int16_t(y_ + (vy_ * gain * dtMs) / 400);

            bounce(x_, vx_, int16_t((f.width() - n) << 8));
            bounce(y_, vy_, int16_t((f.height() - n) << 8));

            f.clear();
            const uint8_t x0 = uint8_t(x_ >> 8);
            const uint8_t y0 = uint8_t(y_ >> 8);
            for (uint8_t i = 0; i < n; ++i)
                for (uint8_t j = 0; j < n; ++j)
                    f.at(uint8_t(x0 + i), uint8_t(y0 + j)) = colour_;
        }

    private:
        void bounce(int16_t& pos, int16_t& vel, int16_t max)
        {
            if (pos < 0) { pos = 0; vel = int16_t(-vel); recolour(); }
            if (pos > max) { pos = max; vel = int16_t(-vel); recolour(); }
        }

        void recolour() { colour_ = CHSV(uint8_t(random8(9) * 28), 255, 255); }

        int16_t x_ = 0; // 8.8 fixed point, pixels
        int16_t y_ = 0;
        int16_t vx_ = 0;
        int16_t vy_ = 0;
        CRGB colour_;
    };
}

REGISTER_EFFECT(Cube, "Блуждающий кубик", ::core::Tag::Dynamic)
