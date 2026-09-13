// Host tests for the x/y -> strip index mapping.
//
// This is the code that is impossible to eyeball on real hardware: a wrong
// mapping produces a picture that looks plausible but is mirrored or
// interleaved. Here all eight valid geometries are checked in one run.

#include <unity.h>

#include <set>

#include "core/Matrix.h"

using namespace core;

namespace {

Geometry geo(Corner c, Direction d, MatrixType t = MatrixType::Serpentine) {
    return Geometry{4, 4, t, c, d};
}

// Whatever the geometry, the mapping must be a bijection onto [0, count).
void assertBijection(const Geometry& g) {
    std::set<uint16_t> seen;
    for (uint8_t y = 0; y < g.height; ++y)
        for (uint8_t x = 0; x < g.width; ++x) {
            const uint16_t i = xyToIndex(x, y, g);
            TEST_ASSERT_LESS_THAN_UINT16(g.count(), i);
            TEST_ASSERT_TRUE_MESSAGE(seen.insert(i).second, "duplicate index");
        }
    TEST_ASSERT_EQUAL_UINT16(g.count(), seen.size());
}

void test_all_geometries_are_bijections() {
    const Corner corners[] = {Corner::BottomLeft, Corner::TopLeft,
                              Corner::TopRight, Corner::BottomRight};
    const Direction dirs[] = {Direction::Right, Direction::Up,
                              Direction::Left, Direction::Down};
    for (Corner c : corners)
        for (Direction d : dirs) {
            for (MatrixType t : {MatrixType::Serpentine, MatrixType::Parallel}) {
                const Geometry g = geo(c, d, t);
                if (!isValid(g)) continue;
                assertBijection(g);
            }
        }
}

// The default matrix: serpentine, first LED bottom-left, strip runs right.
void test_bottom_left_serpentine() {
    const Geometry g = geo(Corner::BottomLeft, Direction::Right);
    TEST_ASSERT_EQUAL_UINT16(0, xyToIndex(0, 0, g));   // first pixel
    TEST_ASSERT_EQUAL_UINT16(3, xyToIndex(3, 0, g));   // end of row 0
    TEST_ASSERT_EQUAL_UINT16(4, xyToIndex(3, 1, g));   // row 1 runs backwards
    TEST_ASSERT_EQUAL_UINT16(7, xyToIndex(0, 1, g));
    TEST_ASSERT_EQUAL_UINT16(8, xyToIndex(0, 2, g));
}

// Parallel wiring has no reversed rows.
void test_parallel_rows_all_forward() {
    const Geometry g = geo(Corner::BottomLeft, Direction::Right, MatrixType::Parallel);
    TEST_ASSERT_EQUAL_UINT16(4, xyToIndex(0, 1, g));
    TEST_ASSERT_EQUAL_UINT16(7, xyToIndex(3, 1, g));
}

// Corner/direction pairs that would send the strip off the panel.
void test_invalid_geometries_are_rejected() {
    TEST_ASSERT_FALSE(isValid(geo(Corner::BottomLeft, Direction::Down)));
    TEST_ASSERT_FALSE(isValid(geo(Corner::TopRight, Direction::Right)));
    TEST_ASSERT_TRUE(isValid(geo(Corner::BottomLeft, Direction::Up)));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_all_geometries_are_bijections);
    RUN_TEST(test_bottom_left_serpentine);
    RUN_TEST(test_parallel_rows_all_forward);
    RUN_TEST(test_invalid_geometries_are_rejected);
    return UNITY_END();
}
