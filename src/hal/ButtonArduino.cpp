// Touch pad, shared by both boards: ttp223 plus EncButton for the gestures.
//
// The pad holds its output high while a finger is on it, so hold and
// multi-click are both available. Gesture meanings live in app/, not here.

#include "hal/Button.h"

#include <Arduino.h>
#include <EncButton.h>

namespace hal
{
    namespace
    {
        class TouchButton final : public Button
        {
        public:
            void begin() override
            {
                btn_.init();
                // Library defaults are 50 / 500 / 600 ms. A single click is only
                // reported once the click window closes (it could still become a
                // double), so the window is what the user feels as lag: 200 ms is
                // the shortest that still lets a relaxed double tap through. The
                // library stores timeouts in 16 ms steps, so these land on 192
                // and 288. The pad debounces in hardware, so the software
                // debounce is nearly free.
                btn_.setDebTimeout(kDebounceMs);
                btn_.setClickTimeout(kClickWindowMs);
                btn_.setHoldTimeout(kHoldStartMs);
            }

            Gesture poll() override
            {
                btn_.tick();

                // hold() with no argument: hold(n) means "held after n clicks"
                // and never fires for a plain press-and-hold.
                if (btn_.hold() && !holding_)
                {
                    holding_ = true;
                    return Gesture::HoldStart;
                }
                if (btn_.holding()) return Gesture::HoldTick;
                if (holding_ && btn_.release())
                {
                    holding_ = false;
                    return Gesture::HoldEnd;
                }

                if (btn_.hasClicks(3)) return Gesture::TripleClick;
                if (btn_.hasClicks(2)) return Gesture::DoubleClick;
                if (btn_.hasClicks(1)) return Gesture::Click;
                return Gesture::None;
            }

        private:
            static constexpr uint8_t kDebounceMs = 20;
            static constexpr uint16_t kClickWindowMs = 200;
            static constexpr uint16_t kHoldStartMs = 300;

            ButtonT<BUTTON_PIN> btn_{INPUT_PULLUP, HIGH};
            bool holding_ = false;
        };
    }

    Button& button()
    {
        static TouchButton instance;
        return instance;
    }
}
