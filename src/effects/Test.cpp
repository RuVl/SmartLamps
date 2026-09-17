// Hardware test patterns. Static, predictable frames for checking the data
// line, the level shifter and the matrix wiring: if "one red pixel" shows
// anything else, the problem is below the effects.

#include <FastLED.h>

#include "core/Registry.h"

namespace
{
    struct Test final : core::Effect
    {
        // 0 black, 1 first pixel red, 2 one white pixel walking by index,
        // 3 all white at 1/255, 4 all white at 16/255, 5 all red, 6 hue by index.
        core::Param pattern{*this, "pattern", "Узор", 0, 6, 1};

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
