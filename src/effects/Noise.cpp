// 3D Perlin noise through a palette: lava, clouds, ocean, plasma and the rest
// of the classic FastLED NoisePlusPalette family. One engine, one Select.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Noise final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 30};
        core::Param scale{*this, "scale", "Масштаб", 1, 100, 25};
        core::Param palette{*this, "palette", "Палитра", core::Select{
            "Плазма;Лава;Облака;Океан;Лес;Радуга;Павлин;Зебра;Безумие", 0}};

        void begin(core::Frame&) override
        {
            x_ = random16();
            y_ = random16();
            z_ = random16();
        }

        void render(core::Frame& f, uint16_t dtMs) override
        {
            const int which = int(palette);
            if (which != loaded_) load(which);

            fill(f, dtMs);
            draw(f, which);
        }

    private:
        enum Which : int { Plasma, Lava, Clouds, Ocean, Forest, RainbowP, Peacock, Zebra, Madness };

        void load(int which)
        {
            switch (which)
            {
            case Lava: pal_ = LavaColors_p; break;
            case Clouds: pal_ = CloudColors_p; break;
            case Ocean: pal_ = OceanColors_p; break;
            case Forest: pal_ = ForestColors_p; break;
            case RainbowP: pal_ = RainbowColors_p; break;
            case Peacock: pal_ = RainbowStripeColors_p; break;
            case Zebra:
                fill_solid(pal_.entries, 16, CRGB::Black);
                pal_[0] = pal_[4] = pal_[8] = pal_[12] = CRGB::White;
                break;
            default: pal_ = PartyColors_p; break;
            }
            // Lava, clouds and the like are meant to sit still in hue; the
            // rainbow family cycles through its palette over time.
            cycle_ = which == RainbowP || which == Peacock || which == Zebra || which == Plasma;
            loaded_ = which;
        }

        void fill(core::Frame& f, uint16_t dtMs)
        {
            const uint16_t sp = uint16_t(speed);
            // Noise scale: small values zoom in on big blobs.
            const uint16_t sc = uint16_t(5 + (int(scale) * 95) / 100);
            // Slow settings get temporal smoothing, as in the original sketch,
            // or the picture shimmers between frames.
            const uint8_t smoothing = sp < 50 ? uint8_t(200 - sp * 4) : 0;

            for (uint8_t i = 0; i < f.width(); ++i)
            {
                const uint16_t io = uint16_t(sc * i);
                for (uint8_t j = 0; j < f.height(); ++j)
                {
                    const uint16_t jo = uint16_t(sc * j);
                    uint8_t data = inoise8(x_ + io, y_ + jo, z_);
                    // Stretch the middling noise range towards the ends.
                    data = qsub8(data, 16);
                    data = qadd8(data, scale8(data, 39));
                    if (smoothing)
                        data = uint8_t(scale8(noise_[i][j], smoothing) + scale8(data, uint8_t(256 - smoothing)));
                    noise_[i][j] = data;
                }
            }

            // Drift: mostly through z, with a slow slide in x and y for variety.
            const uint16_t step = uint16_t((dtMs * sp) / 8);
            z_ = uint16_t(z_ + step);
            x_ = uint16_t(x_ + step / 8);
            y_ = uint16_t(y_ - step / 16);
            hueMs_ = uint16_t(hueMs_ + dtMs);
            if (hueMs_ >= 40) { hueMs_ = 0; ++hue_; }
        }

        void draw(core::Frame& f, int which)
        {
            for (uint8_t x = 0; x < f.width(); ++x)
                for (uint8_t y = 0; y < f.height(); ++y)
                {
                    uint8_t index = noise_[y][x];
                    uint8_t bri = noise_[x][y];
                    if (which == Madness)
                    {
                        f.at(x, y) = CHSV(index, 255, bri);
                        continue;
                    }
                    if (cycle_) index = uint8_t(index + hue_);
                    // Palettes carry their own light/dark range; only dim the low half.
                    bri = bri > 127 ? 255 : dim8_raw(uint8_t(bri * 2));
                    f.at(x, y) = ColorFromPalette(pal_, index, bri);
                }
        }

        uint8_t noise_[MATRIX_WIDTH][MATRIX_HEIGHT] = {};
        CRGBPalette16 pal_{PartyColors_p};
        uint16_t x_ = 0;
        uint16_t y_ = 0;
        uint16_t z_ = 0;
        uint16_t hueMs_ = 0;
        uint8_t hue_ = 0;
        int loaded_ = -1;
        bool cycle_ = true;
    };
}

REGISTER_EFFECT(Noise, "Шум 3D", ::core::Tag::Ambient)
