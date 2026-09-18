// Blizzard: flakes with tails blown sideways by a gusting wind.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Blizzard final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 40};
        core::Param density{*this, "density", "Плотность", 1, 100, 50};
        core::Param tail{*this, "tail", "Шлейф", 0, 100, 50};

        void begin(core::Frame& f) override
        {
            for (Flake& k : flakes_) spawn(k, f, true);
        }

        void render(core::Frame& f, uint16_t dtMs) override
        {
            // The tail is whatever the last frames left behind: the longer it
            // should be, the less each frame fades.
            f.fade(uint8_t(140 - int(tail)));

            gustMs_ = uint16_t(gustMs_ + dtMs);
            if (gustMs_ >= 700)
            {
                gustMs_ = 0;
                wind_ = int8_t(random8(60) - 30);
            }

            // Fall speed in 1/256 px per 16 ms: speed 100 crosses the matrix in ~0.8 s.
            const int16_t vy = int16_t(12 + (int(speed) * 70) / 100);
            const uint8_t active = uint8_t(4 + (int(density) * (kFlakes - 4)) / 100);

            for (uint8_t i = 0; i < active; ++i)
            {
                Flake& k = flakes_[i];
                k.y = int16_t(k.y - (vy * dtMs) / 16);
                k.x = int16_t(k.x + ((int(k.drift) + wind_) * dtMs) / 16);

                const int16_t wrap = int16_t(f.width() << 8);
                if (k.x < 0) k.x = int16_t(k.x + wrap);
                if (k.x >= wrap) k.x = int16_t(k.x - wrap);
                if (k.y < 0) spawn(k, f, false);

                f.at(uint8_t(k.x >> 8), uint8_t(k.y >> 8)) = CHSV(k.hue, 40, 255);
            }
        }

    private:
        static constexpr uint8_t kFlakes = 24;

        struct Flake
        {
            int16_t x; // 8.8 fixed point, pixels
            int16_t y;
            int8_t drift; // own sideways tendency, on top of the wind
            uint8_t hue;
        };

        void spawn(Flake& k, core::Frame& f, bool anywhere)
        {
            k.x = int16_t(random16(uint16_t(f.width() << 8)));
            k.y = anywhere ? int16_t(random16(uint16_t(f.height() << 8)))
                           : int16_t((f.height() << 8) - 1);
            k.drift = int8_t(random8(20) - 10);
            k.hue = random8(140, 200); // cold: cyan through blue
        }

        Flake flakes_[kFlakes] = {};
        int8_t wind_ = 0;
        uint16_t gustMs_ = 0;
    };
}

REGISTER_EFFECT(Blizzard, "Метель", ::core::Tag::Dynamic)
