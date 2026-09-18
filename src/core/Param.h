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
//
// The kind only decides which widget the panel draws. The value, the key, the
// MQTT topic and the JSON field are the same 16-bit integer for every kind, so
// a Switch is 0/1 and a Select is the index of the chosen option.

#include <stdint.h>

#include <atomic>

namespace core
{
    class Effect;

    // Declaration tags. `core::Param x{*this, "k", "Label", core::Hue{20}};`
    // reads as what it is, without a Kind argument that could disagree with
    // the range.
    struct Hue { int16_t def; };           // 0..255 on the colour wheel
    struct Switch { bool def; };           // 0 or 1
    struct Select                          // index into ";"-separated options
    {
        const char* options;               // "Вертикально;Горизонтально"
        int16_t def = 0;
    };

    class Param
    {
    public:
        enum class Kind : uint8_t { Slider, Hue, Switch, Select };

        Param(Effect& owner, const char* key, const char* label,
              int16_t min, int16_t max, int16_t def);
        Param(Effect& owner, const char* key, const char* label, core::Hue hue);
        Param(Effect& owner, const char* key, const char* label, core::Switch sw);
        Param(Effect& owner, const char* key, const char* label, core::Select sel);

        // Reads as a number: `if (hue > 128)`, `f.fade(speed)`. The implicit
        // conversion is deliberate and is what makes effect code read like the
        // arithmetic it is; see docs/writing-effects.md. A Param is a value with
        // no ownership, so the usual danger of implicit conversion does not apply.
        // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
        [[nodiscard]] operator int16_t() const { return value_.load(std::memory_order_relaxed); }

        // Explicit spelling, for where the conversion would be ambiguous.
        [[nodiscard]] int16_t get() const { return value_.load(std::memory_order_relaxed); }

        // Clamps into [min, max]. Returns true when the value actually changed.
        // Not const, although the compiler would allow it: std::atomic::store is
        // const-qualified, but changing a parameter's value is not a const
        // operation on the parameter in any sense a reader would recognise.
        // NOLINTNEXTLINE(readability-make-member-function-const)
        bool set(int16_t v);

        [[nodiscard]] const char* key() const { return key_; }
        [[nodiscard]] const char* label() const { return label_; }
        [[nodiscard]] int16_t min() const { return min_; }
        [[nodiscard]] int16_t max() const { return max_; }
        [[nodiscard]] int16_t def() const { return def_; }
        [[nodiscard]] Kind kind() const { return kind_; }

        // ";"-separated option labels for Kind::Select, nullptr otherwise.
        [[nodiscard]] const char* options() const { return options_; }

        // Reads a Switch as a bool. Same as `x != 0`, spelled for the reader.
        [[nodiscard]] bool on() const { return get() != 0; }

        [[nodiscard]] Param* next() const { return next_; }

    private:
        Param(Effect& owner, const char* key, const char* label, Kind kind,
              int16_t min, int16_t max, int16_t def, const char* options);

        const char* key_;
        const char* label_;
        const char* options_;
        int16_t min_;
        int16_t max_;
        int16_t def_;
        std::atomic<int16_t> value_;
        Kind kind_;
        Param* next_ = nullptr;

        friend class Effect;
    };
}
