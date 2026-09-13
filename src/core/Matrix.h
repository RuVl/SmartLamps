#pragma once
// Physical layout of the LED matrix and the x/y -> strip index mapping.
//
// The geometry is a plain value, not a set of preprocessor macros, so every
// combination can be exercised from the host tests in a single binary. The
// mapping is evaluated once at boot into a lookup table (see Frame); the hot
// render path never recomputes it.

#include <stdint.h>

namespace core {

enum class MatrixType : uint8_t {
    Serpentine,  // every other row runs backwards
    Parallel,    // every row runs in the same direction
};

// Where the first LED of the strip sits, looking at the front of the matrix.
enum class Corner : uint8_t { BottomLeft, TopLeft, TopRight, BottomRight };

// Where the strip goes from that corner.
enum class Direction : uint8_t { Right, Up, Left, Down };

struct Geometry {
    uint8_t width;
    uint8_t height;
    MatrixType type;
    Corner corner;
    Direction direction;

    [[nodiscard]] constexpr uint16_t count() const { return uint16_t(width) * height; }
};

// Not every corner/direction pair describes a real matrix: the strip has to
// leave the corner along an edge, not off the panel.
constexpr bool isValid(const Geometry& g) {
    switch (g.corner) {
        case Corner::BottomLeft:
            return g.direction == Direction::Right || g.direction == Direction::Up;
        case Corner::TopLeft:
            return g.direction == Direction::Right || g.direction == Direction::Down;
        case Corner::TopRight:
            return g.direction == Direction::Left || g.direction == Direction::Down;
        case Corner::BottomRight:
            return g.direction == Direction::Left || g.direction == Direction::Up;
    }
    return false;
}

// Maps matrix coordinates onto the index of the LED in the strip.
// Origin is the bottom-left pixel of the panel as the viewer sees it.
constexpr uint16_t xyToIndex(uint8_t x, uint8_t y, const Geometry& g) {
    const uint8_t w = g.width;
    const uint8_t h = g.height;

    // Rotate the panel coordinates into strip coordinates: `row` counts along
    // the strip's rows, `col` along each row, `rowLen` is the row length.
    uint8_t rowLen = w;
    uint8_t col = x;
    uint8_t row = y;

    switch (g.corner) {
        case Corner::BottomLeft:
            if (g.direction == Direction::Up) { rowLen = h; col = y; row = x; }
            break;
        case Corner::TopLeft:
            if (g.direction == Direction::Down) { rowLen = h; col = uint8_t(h - y - 1); row = x; }
            else                                { row = uint8_t(h - y - 1); }
            break;
        case Corner::TopRight:
            if (g.direction == Direction::Down) { rowLen = h; col = uint8_t(h - y - 1); row = uint8_t(w - x - 1); }
            else                                { col = uint8_t(w - x - 1); row = uint8_t(h - y - 1); }
            break;
        case Corner::BottomRight:
            if (g.direction == Direction::Up) { rowLen = h; col = y; row = uint8_t(w - x - 1); }
            else                              { col = uint8_t(w - x - 1); }
            break;
    }

    const bool forward = (g.type == MatrixType::Parallel) || (row % 2 == 0);
    return (uint16_t(row) * rowLen) + (forward ? col : uint8_t(rowLen - col - 1));
}

// Fills `out` with width*height entries. Call once at boot.
inline void buildIndexMap(uint16_t* out, const Geometry& g) {
    for (uint8_t y = 0; y < g.height; ++y)
        for (uint8_t x = 0; x < g.width; ++x)
            out[(uint16_t(y) * g.width) + x] = xyToIndex(x, y, g);
}

}  // namespace core
