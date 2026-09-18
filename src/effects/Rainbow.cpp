// A rainbow gradient sliding across the matrix.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Rainbow final : core::Effect
    {
        core::Param speed{*this, "speed", "Скорость", 1, 100, 30};
        core::Param scale{*this, "scale", "Масштаб", 1, 100, 40};
        core::Param dir{*this, "dir", "Направление",
                        core::Select{"Вертикально;Горизонтально;По диагонали", 0}};

        void render(core::Frame& f, uint16_t dtMs) override
        {
            // 16.16 hue: speed 100 is one full cycle in about 0.65 s, speed 10
            // in 6.5 s - continuous, no step quantisation at low speeds.
            hue16_ = uint16_t(hue16_ + dtMs * uint16_t(speed));
            const uint8_t base = uint8_t(hue16_ >> 8);
            // Hue change per pixel: at 100 the whole wheel fits the matrix.
            const uint8_t step = uint8_t((int(scale) * 255) / (100 * f.height()));
            const int mode = int(dir);

            for (uint8_t y = 0; y < f.height(); ++y)
                for (uint8_t x = 0; x < f.width(); ++x)
                {
                    const uint8_t along = mode == 0 ? y : mode == 1 ? x : uint8_t(x + y);
                    f.at(x, y) = CHSV(uint8_t(base + along * step), 255, 255);
                }
        }

    private:
        uint16_t hue16_ = 0;
    };
}

REGISTER_EFFECT(Rainbow, "Радуга", ::core::Tag::Ambient)
