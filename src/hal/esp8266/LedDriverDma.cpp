// ESP8266: the strip is driven by I2S DMA through NeoPixelBus.
//
// FastLED's ESP8266 backend bit-bangs WS2812B and blocks interrupts for about
// 7.7 ms per frame, which tears async HTTP responses apart. NeoPixelBus hands
// the frame to DMA instead. The cost is the pin: the DMA method is wired to
// GPIO3 (RX) in hardware and ignores whatever pin it is given.
//
// FastLED stays in the build as the colour library - only the transport
// changes. See docs/adr/0003-hardware-led-transport.md.

#include "hal/LedDriver.h"

#include <NeoPixelBus.h>

namespace hal
{
    namespace
    {
        constexpr uint16_t kPixelCount = MATRIX_WIDTH * MATRIX_HEIGHT;

        // Pixels wired before the matrix that are not part of the picture: a
        // level-shifting "sacrificial" WS2812B fed through a diode reads the
        // ESP's 3.3 V data and re-drives it for the matrix. They stay black.
#ifndef LED_LEAD_PIXELS
#define LED_LEAD_PIXELS 0
#endif
        constexpr uint16_t kLeadPixels = LED_LEAD_PIXELS;

        // A single-transistor level shifter inverts the line; the inverted
        // method pre-inverts the waveform so the strip still sees WS2812 timing.
#ifdef LED_DATA_INVERTED
        using Method = NeoEsp8266DmaInverted800KbpsMethod;
#else
        using Method = NeoEsp8266Dma800KbpsMethod;
#endif
        NeoPixelBus<NeoGrbFeature, Method> bus(kLeadPixels + kPixelCount, LED_DATA_PIN);

        class DmaDriver final : public LedDriver
        {
        public:
            void begin(CRGB* pixels, uint16_t count) override
            {
                pixels_ = pixels;
                count_ = count < kPixelCount ? count : kPixelCount;
                bus.Begin();
                bus.ClearTo(RgbColor(0));
                bus.Show();
            }

            void show(uint8_t brightness) override
            {
                if (pixels_ == nullptr) return;
#ifdef LAMP_MEMLOG
                // Bench only: a picture that stands still while loop() runs
                // means the DMA never reported the previous frame as sent.
                if (!bus.CanShow())
                {
                    ++skipped_;
                    return;
                }
                const uint32_t now = millis();
                if (lastShowMs_ != 0 && now - lastShowMs_ >= 100)
                    Serial.printf("dma: gap %u ms, %u frames skipped\n", unsigned(now - lastShowMs_), unsigned(skipped_));
                lastShowMs_ = now;
                skipped_ = 0;
#else
                if (!bus.CanShow()) return;
#endif
                // Brightness is applied while copying, so the frame the effect drew
                // stays untouched and the next frame starts from full-range colour.
                for (uint16_t i = 0; i < count_; ++i)
                {
                    const CRGB& c = pixels_[i];
                    bus.SetPixelColor(kLeadPixels + i, RgbColor(scale8(c.r, brightness),
                                                                scale8(c.g, brightness),
                                                                scale8(c.b, brightness)));
                }
                bus.Show();
            }

            bool busy() const override { return !bus.CanShow(); }

        private:
            CRGB* pixels_ = nullptr;
            uint16_t count_ = 0;
#ifdef LAMP_MEMLOG
            uint32_t lastShowMs_ = 0;
            uint32_t skipped_ = 0;
#endif
        };
    }

    LedDriver& ledDriver()
    {
        static DmaDriver instance;
        return instance;
    }
}
