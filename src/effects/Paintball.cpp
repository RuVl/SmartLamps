// Paintball: four coloured blobs on sine paths, blurred into each other.
// After FastLED's "light balls" demo, with its own clock instead of millis().

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Paintball final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 50};
        core::Param blur{*this, "blur", "Размытие", 1, 100, 60};

        void render(core::Frame& f, uint16_t dtMs) override
        {
            // Time runs faster or slower with the speed setting; everything
            // below is a function of t, so the picture only depends on it.
            t_ += uint32_t(dtMs) * uint32_t(speed) / 50;

            // The demo dims its 64..100 through dim8_raw, i.e. it really blurs by 16..40.
            blur2d(f, uint8_t(8 + int(blur) / 2));

            const uint8_t w = f.width();
            const uint8_t h = f.height();
            const uint8_t i = wave(91, 1, uint8_t(w - 2));
            const uint8_t j = wave(109, 1, uint8_t(h - 2));
            const uint8_t k = wave(73, 1, uint8_t(w - 2));
            const uint8_t m = wave(123, 1, uint8_t(h - 2));

            // Each blob's hue drifts at its own pace.
            f.at(i, j) += CHSV(uint8_t(t_ / 29), 200, 255);
            f.at(j, k) += CHSV(uint8_t(t_ / 41), 200, 255);
            f.at(k, m) += CHSV(uint8_t(t_ / 73), 200, 255);
            f.at(m, i) += CHSV(uint8_t(t_ / 97), 200, 255);
        }

    private:
        // beatsin8 without the global clock: a sine at `bpm` between lo and hi.
        uint8_t wave(uint16_t bpm, uint8_t lo, uint8_t hi) const
        {
            const uint8_t phase = uint8_t((t_ * bpm * 256UL / 60000UL) & 0xFF);
            return uint8_t(lo + scale8(sin8(phase), uint8_t(hi - lo)));
        }

        // FastLED's blur1d over every row, then every column, through Frame::at
        // so the matrix geometry stays out of the effect.
        static void blur2d(core::Frame& f, fract8 amount)
        {
            const uint8_t keep = uint8_t(255 - amount);
            const uint8_t seep = uint8_t(amount >> 1);
            for (uint8_t y = 0; y < f.height(); ++y)
            {
                CRGB carry = CRGB::Black;
                for (uint8_t x = 0; x < f.width(); ++x)
                {
                    CRGB cur = f.at(x, y);
                    CRGB part = cur;
                    part.nscale8(seep);
                    cur.nscale8(keep);
                    cur += carry;
                    if (x > 0) f.at(uint8_t(x - 1), y) += part;
                    f.at(x, y) = cur;
                    carry = part;
                }
            }
            for (uint8_t x = 0; x < f.width(); ++x)
            {
                CRGB carry = CRGB::Black;
                for (uint8_t y = 0; y < f.height(); ++y)
                {
                    CRGB cur = f.at(x, y);
                    CRGB part = cur;
                    part.nscale8(seep);
                    cur.nscale8(keep);
                    cur += carry;
                    if (y > 0) f.at(x, uint8_t(y - 1)) += part;
                    f.at(x, y) = cur;
                    carry = part;
                }
            }
        }

        uint32_t t_ = 0;
    };
}

REGISTER_EFFECT(Paintball, "Пейнтбол", ::core::Tag::Dynamic)
