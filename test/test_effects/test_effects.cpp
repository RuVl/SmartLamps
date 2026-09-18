// Smoke test for every registered effect: build it in an arena of the real
// size, run it for a few seconds of simulated time at odd frame intervals, and
// check that it never writes outside the pixel buffer. This is what catches an
// off-by-one in a particle effect before it corrupts the lamp's heap.

#include <unity.h>

#include <FastLED.h>

#include <string.h>

#include "core/Frame.h"
#include "core/Matrix.h"
#include "core/Registry.h"

using namespace core;

namespace {
    constexpr uint16_t kCount = MATRIX_WIDTH * MATRIX_HEIGHT;
    constexpr uint16_t kGuard = 16; // pixels of canary on each side of the buffer

    struct Canvas {
        CRGB pixels[kGuard + kCount + kGuard];
        uint16_t map[kCount];
        Geometry geometry{MATRIX_WIDTH, MATRIX_HEIGHT, MatrixType::Serpentine,
                          Corner::BottomLeft, Direction::Right};

        Canvas() {
            buildIndexMap(map, geometry);
            fill_solid(pixels, kGuard + kCount + kGuard, CRGB(1, 2, 3)); // canary value
        }

        Frame frame() { return Frame(pixels + kGuard, map, geometry); }

        bool guardsIntact() const {
            for (uint16_t i = 0; i < kGuard; ++i) {
                if (pixels[i] != CRGB(1, 2, 3)) return false;
                if (pixels[kGuard + kCount + i] != CRGB(1, 2, 3)) return false;
            }
            return true;
        }
    };

    alignas(8) uint8_t arena[EFFECT_ARENA_SIZE];

    void runEffect(EffectInfo* info) {
        Canvas canvas;
        Frame f = canvas.frame();
        f.clear();

        Effect* e = info->construct(arena);
        e->begin(f);

        // Uneven frame intervals, including a stall, over ~5 s of lamp time.
        const uint16_t dts[] = {16, 17, 16, 33, 5, 16, 200, 16, 1000, 16};
        for (int i = 0; i < 300; ++i) e->render(f, dts[i % 10]);

        // Every param at its extremes must be survivable too.
        for (Param* p = e->params(); p != nullptr; p = p->next()) {
            p->set(p->max());
            for (int i = 0; i < 30; ++i) e->render(f, 16);
            p->set(p->min());
            for (int i = 0; i < 30; ++i) e->render(f, 16);
            p->set(p->def());
        }
        e->~Effect();

        TEST_ASSERT_TRUE_MESSAGE(canvas.guardsIntact(), info->name);
    }

    void test_every_effect_stays_in_bounds() {
        uint16_t n = 0;
        for (EffectInfo* e = Registry::head(); e != nullptr; e = e->next, ++n) runEffect(e);
        TEST_ASSERT_GREATER_THAN_UINT16(1, n);
    }

    void test_registry_has_expected_effects() {
        TEST_ASSERT_NOT_NULL(Registry::find("Огонь"));
        TEST_ASSERT_NOT_NULL(Registry::find("Шум 3D"));
        TEST_ASSERT_NOT_NULL(Registry::find("Тест"));
        TEST_ASSERT_NULL(Registry::find("Нет такого"));
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_registry_has_expected_effects);
    RUN_TEST(test_every_effect_stays_in_bounds);
    return UNITY_END();
}
