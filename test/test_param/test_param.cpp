// Host tests for Param kinds: the range each kind derives from its declaration
// and the clamping every write goes through.

#include <unity.h>

#include "core/Effect.h"

using namespace core;

namespace {
    struct Probe final : Effect {
        Param slider{*this, "s", "S", 1, 100, 40};
        Param hue{*this, "h", "H", Hue{300}};   // out-of-range default is clamped
        Param sw{*this, "w", "W", Switch{true}};
        Param sel{*this, "e", "E", Select{"A;B;C", 1}};
        Param one{*this, "o", "O", Select{"Only"}};

        void render(Frame&, uint16_t) override {}
    };

    Probe probe;

    void test_kinds_and_ranges() {
        TEST_ASSERT_EQUAL(int(Param::Kind::Slider), int(probe.slider.kind()));
        TEST_ASSERT_EQUAL_INT16(1, probe.slider.min());
        TEST_ASSERT_EQUAL_INT16(100, probe.slider.max());

        TEST_ASSERT_EQUAL(int(Param::Kind::Hue), int(probe.hue.kind()));
        TEST_ASSERT_EQUAL_INT16(0, probe.hue.min());
        TEST_ASSERT_EQUAL_INT16(255, probe.hue.max());
        TEST_ASSERT_EQUAL_INT16(255, probe.hue.def());

        TEST_ASSERT_EQUAL(int(Param::Kind::Switch), int(probe.sw.kind()));
        TEST_ASSERT_EQUAL_INT16(1, probe.sw.max());
        TEST_ASSERT_TRUE(probe.sw.on());

        TEST_ASSERT_EQUAL(int(Param::Kind::Select), int(probe.sel.kind()));
        TEST_ASSERT_EQUAL_INT16(2, probe.sel.max());
        TEST_ASSERT_EQUAL_INT16(1, probe.sel.def());
        TEST_ASSERT_EQUAL_STRING("A;B;C", probe.sel.options());
        TEST_ASSERT_EQUAL_INT16(0, probe.one.max());
    }

    void test_set_clamps_per_kind() {
        TEST_ASSERT_FALSE(probe.sw.set(7)); // clamps to 1, which it already is
        TEST_ASSERT_EQUAL_INT16(1, probe.sw.get());
        TEST_ASSERT_TRUE(probe.sw.set(-3));
        TEST_ASSERT_FALSE(probe.sw.on());

        TEST_ASSERT_TRUE(probe.sel.set(9));
        TEST_ASSERT_EQUAL_INT16(2, probe.sel.get());
        TEST_ASSERT_FALSE(probe.sel.set(2)); // unchanged reports false

        TEST_ASSERT_TRUE(probe.hue.set(-1));
        TEST_ASSERT_EQUAL_INT16(0, probe.hue.get());
    }

    // Declaration order is what the panel and the JSON state rely on.
    void test_declaration_order() {
        Param* p = probe.params();
        TEST_ASSERT_EQUAL_STRING("s", p->key());
        p = p->next();
        TEST_ASSERT_EQUAL_STRING("h", p->key());
        p = p->next()->next()->next();
        TEST_ASSERT_EQUAL_STRING("o", p->key());
        TEST_ASSERT_NULL(p->next());
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_kinds_and_ranges);
    RUN_TEST(test_set_clamps_per_kind);
    RUN_TEST(test_declaration_order);
    return UNITY_END();
}
