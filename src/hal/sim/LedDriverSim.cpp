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

            static void snapshot(uint8_t* out, const uint16_t* indexMap)
            {
                constexpr uint16_t kPixels = MATRIX_WIDTH * MATRIX_HEIGHT;
                for (uint16_t i = 0; i < kPixels; ++i)
                {
                    // i walks the matrix; the map says where that cell sits on the strip.
                    const uint16_t s = indexMap[i];
                    const bool have = pixels_ != nullptr && s < count_;
                    const CRGB c = have ? pixels_[s] : CRGB::Black;
                    out[i * 3 + 0] = scale8(c.r, brightness_);
                    out[i * 3 + 1] = scale8(c.g, brightness_);
                    out[i * 3 + 2] = scale8(c.b, brightness_);
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

    void simSnapshot(uint8_t* out, const uint16_t* indexMap) { SimDriver::snapshot(out, indexMap); }
}
