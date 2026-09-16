#pragma once
// The canvas an effect draws on.
//
// Frame owns nothing: the pixel buffer and the index map live in the render
// task. An effect never touches the strip, never calls show(), and never
// indexes the raw buffer by coordinates - it goes through at()/xy().

#include <stdint.h>

#include <FastLED.h>

#include "Matrix.h"

namespace core
{
    class Frame
    {
    public:
        Frame(CRGB* pixels, const uint16_t* indexMap, const Geometry& geometry)
            : pixels_(pixels), map_(indexMap), geometry_(geometry)
        {
        }

        [[nodiscard]] uint8_t width() const { return geometry_.width; }
        [[nodiscard]] uint8_t height() const { return geometry_.height; }
        [[nodiscard]] uint16_t count() const { return geometry_.count(); }

        // Strip index of a matrix coordinate. Out-of-range coordinates clamp to
        // the last pixel rather than corrupting memory.
        [[nodiscard]] uint16_t xy(uint8_t x, uint8_t y) const
        {
            if (x >= geometry_.width || y >= geometry_.height) return count() - 1;
            return map_[(uint16_t(y) * geometry_.width) + x];
        }

        [[nodiscard]] CRGB& at(uint8_t x, uint8_t y) const { return pixels_[xy(x, y)]; }

        // Linear access, for effects that do not care about geometry.
        [[nodiscard]] CRGB& operator[](uint16_t i) const
        {
            return pixels_[i < count() ? i : count() - 1];
        }

        [[nodiscard]] CRGB* raw() const { return pixels_; }

        // These are const because a Frame is a view: they change the pixels it
        // refers to, not the Frame itself - the same rule std::span follows.
        void clear() const { fill(CRGB::Black); }
        void fill(CRGB color) const { fill_solid(pixels_, count(), color); }
        void fade(uint8_t amount) const { fadeToBlackBy(pixels_, count(), amount); }

        // Scales the whole frame, used when fading between effects.
        void scale(uint8_t factor) const { nscale8(pixels_, count(), factor); }

    private:
        CRGB* pixels_;
        const uint16_t* map_;
        Geometry geometry_;
    };
}
