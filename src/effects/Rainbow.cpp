#include <FastLED.h>

#include "core/Registry.h"


namespace
{
    struct Rainbow final : core::Effect
    {

        void begin()
        {

        }

        void render(core::Frame& frame, uint16_t dtMs) override
        {

        }

    };
}

REGISTER_EFFECT(Rainbow, "Радуга", core::Tag::Ambient)
