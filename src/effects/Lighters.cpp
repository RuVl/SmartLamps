// Fireflies: coloured dots wandering around, optionally leaving trails.

#include <FastLED.h>

#include <algorithm>

#include "core/Registry.h"

namespace
{
    struct Lighters final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 40};
        core::Param count{*this, "count", "Количество", 1, 12, 6};
        core::Param trace{*this, "trace", "Со шлейфом", core::Switch{false}};

        void begin(core::Frame& f) override
        {
            for (Fly& y : flies_)
            {
                y.x = int16_t(random16(uint16_t(f.width() << 8)));
                y.y = int16_t(random16(uint16_t(f.height() << 8)));
                y.vx = int8_t(random8(40) - 20);
                y.vy = int8_t(random8(40) - 20);
                y.hue = random8();
            }
        }

        void render(core::Frame& f, uint16_t dtMs) override
        {
            if (trace.on()) f.fade(uint8_t(dtMs * 2));
            else f.clear();

            // Every ~300 ms each fly picks a slightly different heading.
            nudgeMs_ = uint16_t(nudgeMs_ + dtMs);
            const bool nudge = nudgeMs_ >= 300;
            if (nudge) nudgeMs_ = 0;

            const int16_t xMax = int16_t(f.width() << 8);
            const int16_t yMax = int16_t((f.height() << 8) - 1);
            const int gain = int(speed); // at 100, vx=20 moves ~30 px/s

            for (int i = 0; i < int(count); ++i)
            {
                Fly& y = flies_[i];
                if (nudge)
                {
                    y.vx = int8_t(std::clamp(y.vx + int(random8(7)) - 3, -20, 20));
                    y.vy = int8_t(std::clamp(y.vy + int(random8(7)) - 3, -20, 20));
                }
                y.x = int16_t(y.x + (int(y.vx) * gain * dtMs) / 400);
                y.y = int16_t(y.y + (int(y.vy) * gain * dtMs) / 400);

                // Wrap around horizontally (the lamp is a cylinder), bounce vertically.
                if (y.x < 0) y.x = int16_t(y.x + xMax);
                if (y.x >= xMax) y.x = int16_t(y.x - xMax);
                if (y.y < 0) { y.y = 0; y.vy = int8_t(-y.vy); }
                if (y.y > yMax) { y.y = yMax; y.vy = int8_t(-y.vy); }

                f.at(uint8_t(y.x >> 8), uint8_t(y.y >> 8)) = CHSV(y.hue, 255, 255);
            }
        }

    private:
        struct Fly
        {
            int16_t x; // 8.8 fixed point, pixels
            int16_t y;
            int8_t vx;
            int8_t vy;
            uint8_t hue;
        };

        Fly flies_[12] = {};
        uint16_t nudgeMs_ = 0;
    };
}

REGISTER_EFFECT(Lighters, "Светлячки", ::core::Tag::Dynamic)
