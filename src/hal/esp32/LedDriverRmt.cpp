// ESP32-S3: the strip is driven by the RMT peripheral.
//
// FastLED hands the frame to RMT, which clocks the bits out with hardware
// timing. Interrupts stay enabled throughout - which is the whole reason the
// lamp moved to this chip. See docs/adr/0003-hardware-led-transport.md.

#include "hal/LedDriver.h"

#include <FastLED.h>

namespace hal
{
    namespace
    {
        class RmtDriver final : public LedDriver
        {
        public:
            void begin(CRGB* pixels, uint16_t count) override
            {
                // The pin must be a compile-time constant for FastLED's template.
                FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(pixels, count);
                FastLED.clear(true);
            }

            void show(uint8_t brightness) override
            {
                FastLED.setBrightness(brightness);
                FastLED.show();
            }

            // FastLED's RMT backend waits for the previous transfer inside show(), so
            // from the caller's point of view the driver is never busy.
            bool busy() const override { return false; }
        };
    }

    LedDriver& ledDriver()
    {
        static RmtDriver instance;
        return instance;
    }
}
