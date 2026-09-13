#pragma once
// Everything applied to a finished frame before it reaches the strip.
//
// Effects write raw colour and know nothing about brightness, gamma or the
// power supply. Those belong here, in one place, where they can be tested.

#include <stdint.h>

#include <FastLED.h>

namespace core {
    // Perceptual brightness: a slider at 50% should look half as bright, which a
    // linear scale does not deliver. GAMMA 1.5 matches the previous firmware.
    uint8_t gammaCorrect(uint8_t percent);

    // Estimated draw of the frame at the given brightness, in milliamps. Used to
    // keep the matrix inside what the power supply can deliver.
    uint32_t estimateCurrent(const CRGB *pixels, uint16_t count, uint8_t brightness);

    // Highest brightness that keeps the frame under `limitMa`.
    uint8_t limitBrightness(const CRGB *pixels, uint16_t count,
                            uint8_t brightness, uint16_t limitMa);
} // namespace core
