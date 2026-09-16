#include "hal/LedDriver.h"

#include "SimFrame.h"

namespace hal
{
    namespace
    {
        class SimDriver final : public LedDriver
        {
        public:
            void begin(CRGB* pixels, uint16_t count) override
            {
                pixels_ = pixels;
                count_ = count;
            }

            // The lamp renders full-range; brightness lands here, exactly where
            // FastLED.setBrightness() would apply it on hardware.
            void show(uint8_t brightness) override { brightness_ = brightness; }

            bool busy() const override { return false; }

            static void snapshot(uint8_t* out)
            {
                constexpr uint16_t kPixels = MATRIX_WIDTH * MATRIX_HEIGHT;
                uint16_t n = count_ < kPixels ? count_ : kPixels;
                uint16_t i = 0;
                if (pixels_ != nullptr)
                {
                    for (; i < n; ++i)
                    {
                        out[i * 3 + 0] = scale8(pixels_[i].r, brightness_);
                        out[i * 3 + 1] = scale8(pixels_[i].g, brightness_);
                        out[i * 3 + 2] = scale8(pixels_[i].b, brightness_);
                    }
                }
                for (; i < kPixels; ++i)
                {
                    out[i * 3 + 0] = 0;
                    out[i * 3 + 1] = 0;
                    out[i * 3 + 2] = 0;
                }
            }

        private:
            static inline CRGB* pixels_ = nullptr;
            static inline uint16_t count_ = 0;
            static inline uint8_t brightness_ = 0;
        };
    }

    LedDriver& ledDriver()
    {
        static SimDriver instance;
        return instance;
    }

    void simSnapshot(uint8_t* out) { SimDriver::snapshot(out); }
}
