#pragma once
// The touch pad, reduced to the gestures the lamp actually reacts to.
//
// ttp223 holds its output high while a finger is on the pad, so hold and
// multi-click are both available. Debouncing and gesture detection live in
// the implementation; the application only sees these events.

#include <stdint.h>

namespace hal
{
    enum class Gesture : uint8_t
    {
        None,
        Click, // power on/off
        DoubleClick, // next effect
        TripleClick, // ping the paired lamp (reserved, see docs/mqtt.md)
        HoldStart, // begin brightness adjustment
        HoldTick, // continue adjusting while held
        HoldEnd,
    };

    class Button
    {
    public:
        virtual ~Button() = default;

        virtual void begin() = 0;

        virtual Gesture poll() = 0;
    };

    // For logs.

    inline const char* gestureName(Gesture g)
    {
        switch (g)
        {
        case Gesture::Click: return "click";

        case Gesture::DoubleClick: return "double";

        case Gesture::TripleClick: return "triple";

        case Gesture::HoldStart: return "hold-start";

        case Gesture::HoldTick: return "hold";

        case Gesture::HoldEnd: return "hold-end";

        default: return "none";
        }
    }


    Button& button();
}
