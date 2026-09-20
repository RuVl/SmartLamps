// Host tests for the brightness curve.
//
// The percent -> 0..255 mapping is what the slider, the button sweep and the
// MQTT command all go through; an off-by-one at either end is a lamp that
// never goes fully dark or one that goes dark one step too early.

#include <unity.h>

#include "core/PostFX.h"

using namespace core;

namespace {
    void test_ends_of_the_scale() {
        TEST_ASSERT_EQUAL_UINT8(0, gammaCorrect(0));
        TEST_ASSERT_EQUAL_UINT8(255, gammaCorrect(100));
        TEST_ASSERT_EQUAL_UINT8(255, gammaCorrect(200)); // clamped, not wrapped
    }

    // 1 % rounds to zero on the gamma curve; the lamp must still light there.
    void test_lowest_setting_is_lit() {
        TEST_ASSERT_EQUAL_UINT8(1, gammaCorrect(1));
        TEST_ASSERT_EQUAL_UINT8(1, gammaCorrect(2));
    }

    void test_monotonic() {
        uint8_t prev = gammaCorrect(0);
        for (uint8_t p = 1; p <= 100; ++p) {
            const uint8_t v = gammaCorrect(p);
            TEST_ASSERT_GREATER_OR_EQUAL_UINT8(prev, v);
            prev = v;
        }
    }

    // Half the slider is half the light to the eye, i.e. well under half in linear terms.
    void test_perceptual_midpoint() {
        const uint8_t mid = gammaCorrect(50);
        TEST_ASSERT_GREATER_THAN_UINT8(80, mid);
        TEST_ASSERT_LESS_THAN_UINT8(100, mid);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ends_of_the_scale);
    RUN_TEST(test_lowest_setting_is_lit);
    RUN_TEST(test_monotonic);
    RUN_TEST(test_perceptual_midpoint);
    return UNITY_END();
}
