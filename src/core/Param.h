#pragma once
// A tunable knob of an effect.
//
// One declaration produces five things: the widget in the web panel, the key
// in the settings database, the MQTT topic, the entry in the JSON state and
// the default value. Nothing about a parameter is written twice.
//
// Values are plain 16-bit integers held in an atomic. The render task reads
// them without locking; the network and UI tasks write them. A single 16-bit
// store is atomic on both targets, so a snapshot is never half-updated.

#include <stdint.h>

#include <atomic>

namespace core {

class Effect;

class Param {
public:
    Param(Effect& owner, const char* key, const char* label,
          int16_t min, int16_t max, int16_t def);

    // Reads as a number: `if (hue > 128)`, `f.fade(speed)`. The implicit
    // conversion is deliberate and is what makes effect code read like the
    // arithmetic it is; see docs/writing-effects.md. A Param is a value with
    // no ownership, so the usual danger of implicit conversion does not apply.
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    [[nodiscard]] operator int16_t() const { return value_.load(std::memory_order_relaxed); }

    // Explicit spelling, for where the conversion would be ambiguous.
    [[nodiscard]] int16_t get() const { return value_.load(std::memory_order_relaxed); }

    // Clamps into [min, max]. Returns true when the value actually changed.
    bool set(int16_t v);

    [[nodiscard]] const char* key() const { return key_; }
    [[nodiscard]] const char* label() const { return label_; }
    [[nodiscard]] int16_t min() const { return min_; }
    [[nodiscard]] int16_t max() const { return max_; }
    [[nodiscard]] int16_t def() const { return def_; }

    [[nodiscard]] Param* next() const { return next_; }

private:
    const char* key_;
    const char* label_;
    int16_t min_;
    int16_t max_;
    int16_t def_;
    std::atomic<int16_t> value_;
    Param* next_ = nullptr;

    friend class Effect;
};

}  // namespace core
