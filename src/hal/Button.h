#pragma once
// The touch pad, reduced to the gestures the lamp actually reacts to.
//
// ttp223 holds its output high while a finger is on the pad, so hold and
// multi-click are both available. Debouncing and gesture detection live in
// the implementation; the application only sees these events.

#include <stdint.h>

namespace hal {

enum class Gesture : uint8_t {
    None,
    Click,        // next effect
    DoubleClick,  // previous effect
    TripleClick,  // ping the paired lamp
    HoldStart,    // begin brightness adjustment
    HoldTick,     // continue adjusting while held
    HoldEnd,
    LongHold,     // 5 s: toggle the lamp on/off
};

class Button {
public:
    virtual ~Button() = default;
    virtual void begin() = 0;
    virtual Gesture poll() = 0;
};

Button& button();

}  // namespace hal
