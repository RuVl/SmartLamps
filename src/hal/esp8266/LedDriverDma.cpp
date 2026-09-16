// ESP8266: the strip is driven by I2S DMA through NeoPixelBus.
//
// FastLED's ESP8266 backend bit-bangs WS2812B and blocks interrupts for about
// 7.7 ms per frame, which tears async HTTP responses apart. NeoPixelBus hands
// the frame to DMA instead. The cost is the pin: the DMA method is wired to
// GPIO3 (RX) in hardware and ignores whatever pin it is given.
//
// FastLED stays in the build as the colour library — only the transport
// changes. See docs/adr/0003-hardware-led-transport.md.

#include "hal/LedDriver.h"

#include <NeoPixelBus.h>

namespace hal
{
    namespace
    {
        constexpr uint16_t kPixelCount = MATRIX_WIDTH * MATRIX_HEIGHT;

        NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> bus(kPixelCount, LED_DATA_PIN);

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
                if (pixels_ == nullptr || !bus.CanShow()) return;
                // Brightness is applied while copying, so the frame the effect drew
                // stays untouched and the next frame starts from full-range colour.
                for (uint16_t i = 0; i < count_; ++i)
                {
                    const CRGB& c = pixels_[i];
                    bus.SetPixelColor(i, RgbColor(scale8(c.r, brightness),
                                                  scale8(c.g, brightness),
                                                  scale8(c.b, brightness)));
                }
                bus.Show();
            }

            bool busy() const override { return !bus.CanShow(); }

        private:
            CRGB* pixels_ = nullptr;
            uint16_t count_ = 0;
        };
    }

    LedDriver& ledDriver()
    {
        static DmaDriver instance;
        return instance;
    }
}
