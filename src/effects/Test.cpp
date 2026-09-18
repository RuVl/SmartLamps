// Hardware test patterns. Static, predictable frames for checking the data
// line, the level shifter and the matrix wiring: if "one red pixel" shows
// anything else, the problem is below the effects.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Test final : core::Effect
    {
        // "White 1/255" is the near-all-zero frame: the hardest case for the
        // WS2812B zero-pulse timing, the one that lights random pixels when the
        // level shifter is marginal.
        core::Param pattern{*this, "pattern", "Узор", core::Select{
            "Чёрный;Один красный;Бегущий белый;Белый 1/255;Белый 16/255;Красный;Оттенок по индексу", 1}};

        void render(core::Frame& f, uint16_t dtMs) override
        {
            f.clear();
            switch (int(pattern))
            {
                case 1: f[0] = CRGB::Red; break;
                case 2:
                    accumulator_ += dtMs;
                    while (accumulator_ >= 100)
                    {
                        accumulator_ -= 100;
                        walker_ = uint16_t((walker_ + 1) % f.count());
                    }
                    f[walker_] = CRGB::White;
                    break;
                case 3: f.fill(CRGB(1, 1, 1)); break;
                case 4: f.fill(CRGB(16, 16, 16)); break;
                case 5: f.fill(CRGB::Red); break;
                case 6:
                    for (uint16_t i = 0; i < f.count(); ++i) f[i] = CHSV(uint8_t(i), 255, 255);
                    break;
                default: break;
            }
        }

    private:
        uint16_t walker_ = 0;
        uint16_t accumulator_ = 0;
    };
}

REGISTER_EFFECT(Test, "Тест", ::core::Tag::System)
