// Touch pad, shared by both boards: ttp223 plus EncButton for the gestures.
//
// The pad holds its output high while a finger is on it, so hold and
// multi-click are both available. Gesture meanings live in app/, not here.

#include "hal/Button.h"

#include <Arduino.h>
#include <EncButton.h>

namespace hal {
namespace {

class TouchButton final : public Button {
public:
    void begin() override { btn_.init(); }

    Gesture poll() override {
        btn_.tick();

        if (btn_.hold(1) && !holding_) { holding_ = true; return Gesture::HoldStart; }
        if (btn_.holding()) {
            // Five seconds of holding means power, not brightness. The
            // brightness ramp stops as soon as that threshold is crossed.
            if (!longFired_ && btn_.pressFor() >= kLongHoldMs) {
                longFired_ = true;
                return Gesture::LongHold;
            }
            return longFired_ ? Gesture::None : Gesture::HoldTick;
        }
        if (holding_ && btn_.release()) {
            holding_ = false;
            const bool wasLong = longFired_;
            longFired_ = false;
            return wasLong ? Gesture::None : Gesture::HoldEnd;
        }

        if (btn_.hasClicks(3)) return Gesture::TripleClick;
        if (btn_.hasClicks(2)) return Gesture::DoubleClick;
        if (btn_.hasClicks(1)) return Gesture::Click;
        return Gesture::None;
    }

private:
    static constexpr uint16_t kLongHoldMs = 5000;

    ButtonT<BUTTON_PIN> btn_{INPUT_PULLUP, HIGH};
    bool holding_ = false;
    bool longFired_ = false;
};

}  // namespace

Button& button() {
    static TouchButton instance;
    return instance;
}

}  // namespace hal
