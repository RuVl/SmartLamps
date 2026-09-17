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
#if defined(LED_DATA_INVERTED) && defined(LED_DEBUG_STEPS)
        // Debug only: LED_DEBUG_STEPS I2S steps per bit, a zero high for
        // LED_DEBUG_ZERO steps and a one high for LED_DEBUG_ONE steps, to
        // measure how much a slow level shifter eats from each pulse.
        class InvertedStepsEncoder
        {
        public:
            static const uint8_t IdleLevel = 1;
            static const size_t DmaBitsPerPixelBit = LED_DEBUG_STEPS;
            static size_t SpacingPixelSize(size_t sizePixel) { return sizePixel; }
            static void FillBuffers(uint8_t* i2sBuffer, const uint8_t* data, size_t sizeData, size_t)
            {
                // RX low = DIN high: a bit is `high` zeros then ones. MSB first into 32-bit words.
                constexpr uint8_t steps = LED_DEBUG_STEPS;
                uint32_t* pDma = reinterpret_cast<uint32_t*>(i2sBuffer);
                uint32_t word = 0;
                uint8_t left = 32;
                for (size_t i = 0; i < sizeData; ++i)
                {
                    uint8_t value = data[i];
                    for (uint8_t b = 0; b < 8; ++b, value <<= 1)
                    {
                        const uint8_t high = (value & 0x80) ? LED_DEBUG_ONE : LED_DEBUG_ZERO;
                        for (uint8_t k = 0; k < steps; ++k)
                        {
                            --left;
                            if (k >= high) word |= 1u << left;
                            if (left == 0) { *pDma++ = word; word = 0; left = 32; }
                        }
                    }
                }
                if (left != 32) *pDma++ = word | (0xFFFFFFFFu >> (32 - left)); // pad with idle
            }
        };
        using Method = NeoEsp8266DmaMethodBase<InvertedStepsEncoder, NeoBitsSpeed800Kbps>;
#elif defined(LED_DATA_INVERTED) && defined(LED_DEBUG_400K)
        // Debug only: half-speed bits, to tell a slow level shifter from a dead one.
        using Method = NeoEsp8266DmaInverted400KbpsMethod;
#elif defined(LED_DATA_INVERTED)
        using Method = NeoEsp8266DmaInverted800KbpsMethod;
#else
        using Method = NeoEsp8266Dma800KbpsMethod;
#endif
        NeoPixelBus<NeoGrbFeature, Method> bus(kLeadPixels + kPixelCount, LED_DATA_PIN);

#ifdef LAMP_DEBUG_HTTP
        // Debug: park the data pin at a DC level so the level shifter can be
        // checked with a multimeter, no timing involved.
        bool g_debugHold = false;
        uint32_t g_debugShown = 0; // frames actually handed to DMA
        uint32_t g_debugSkipped = 0; // show() calls refused by CanShow()
#endif

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
#ifdef LAMP_DEBUG_HTTP
                if (g_debugHold) return;
#endif
#ifdef LAMP_DEBUG_HTTP
                if (!bus.CanShow()) { ++g_debugSkipped; return; }
                ++g_debugShown;
#endif
                if (pixels_ == nullptr || !bus.CanShow()) return;
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
        };
    }

    LedDriver& ledDriver()
    {
        static DmaDriver instance;
        return instance;
    }

#ifdef LAMP_DEBUG_HTTP
    String ledDebugStats()
    {
        String s;
        s += F(" shown="); s += g_debugShown;
        s += F(" skipped="); s += g_debugSkipped;
        s += F(" canShow="); s += bus.CanShow();
        s += F(" hold="); s += g_debugHold;
        s += F(" gpio3="); s += digitalRead(LED_DATA_PIN);
        return s;
    }

    void ledDebugHold(int level)
    {
        if (level < 0)
        {
            // Back to I2S. A PWM waveform or a GPIO output left on the pin
            // keeps driving it even after Begin() re-selects the I2S function,
            // so drop the pin back to a plain input first.
            digitalWrite(LED_DATA_PIN, LOW); // stops the waveform generator
            pinMode(LED_DATA_PIN, INPUT);
            bus.Begin();
            g_debugHold = false;
            return;
        }
        g_debugHold = true;
        while (!bus.CanShow()) delay(1); // let the last frame leave the pin
        pinMode(LED_DATA_PIN, OUTPUT);
        if (level > 1)
        {
            // 50 % square wave at `level` Hz, to check the pin toggles at all.
            analogWriteRange(1023);
            analogWriteFreq(uint32_t(level));
            analogWrite(LED_DATA_PIN, 512);
            return;
        }
        digitalWrite(LED_DATA_PIN, level ? HIGH : LOW);
    }
#endif
}
