#include "PostFX.h"

#include <math.h>

namespace core
{
    namespace
    {
        // Matches the previous firmware; 1.0 would be linear and looks wrong.
        constexpr float kGamma = 1.5f;
        constexpr uint32_t kSupplyVolts = 5;
    }

    uint8_t gammaCorrect(uint8_t percent)
    {
        if (percent == 0) return 0;
        if (percent >= 100) return 255;
        // lroundf, not "+ 0.5 and truncate": the latter rounds incorrectly at the
        // representable edges and is a classic off-by-one source.
        return uint8_t(lroundf(powf(float(percent) / 100.0f, kGamma) * 255.0f));
    }

    uint32_t estimateCurrent(const CRGB* pixels, uint16_t count, uint8_t brightness)
    {
        const uint32_t mW = calculate_unscaled_power_mW(pixels, count);
        return ((mW * brightness) / 255) / kSupplyVolts;
    }

    uint8_t limitBrightness(const CRGB* pixels, uint16_t count,
                            uint8_t brightness, uint16_t limitMa)
    {
        if (limitMa == 0) return brightness;
        return calculate_max_brightness_for_power_vmA(pixels, count, brightness,
                                                      kSupplyVolts, limitMa);
    }
}
