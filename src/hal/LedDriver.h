#pragma once
// Transport to the LED strip. The one place that knows about the hardware.
//
// ESP32-S3 drives the strip from the RMT peripheral, ESP8266 from I2S DMA via
// NeoPixelBus. Both hand the frame to hardware and return immediately without
// blocking interrupts - which is what keeps the async web server intact. See
// docs/adr/0003-hardware-led-transport.md.

#include <stdint.h>

#include <FastLED.h>

namespace hal
{
    class LedDriver
    {
    public:
        virtual ~LedDriver() = default;

        virtual void begin(CRGB* pixels, uint16_t count) = 0;

        // Hands the buffer to the hardware. Returns without waiting for the last
        // bit to leave the pin.
        virtual void show(uint8_t brightness) = 0;

        // True while the previous frame is still being clocked out.
        [[nodiscard]] virtual bool busy() const = 0;
    };

    LedDriver& ledDriver();
}
