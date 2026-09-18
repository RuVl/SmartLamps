#include "Effect.h"

#include <algorithm>

namespace core
{
    namespace
    {
        // "A;B;C" -> 3. An empty string still counts as one (empty) option, so a
        // Select can never have a max below its min.
        int16_t countOptions(const char* options)
        {
            int16_t n = 1;
            for (const char* c = options; *c != 0; ++c)
                if (*c == ';') ++n;
            return n;
        }
    }

    Param::Param(Effect& owner, const char* key, const char* label, Kind kind,
                 int16_t min, int16_t max, int16_t def, const char* options)
        : key_(key), label_(label), options_(options), min_(min), max_(max),
          def_(std::clamp(def, min, max)), value_(def_), kind_(kind)
    {
        owner.addParam(this);
    }

    Param::Param(Effect& owner, const char* key, const char* label,
                 int16_t min, int16_t max, int16_t def)
        : Param(owner, key, label, Kind::Slider, min, max, def, nullptr)
    {
    }

    Param::Param(Effect& owner, const char* key, const char* label, core::Hue hue)
        : Param(owner, key, label, Kind::Hue, 0, 255, hue.def, nullptr)
    {
    }

    Param::Param(Effect& owner, const char* key, const char* label, core::Switch sw)
        : Param(owner, key, label, Kind::Switch, 0, 1, sw.def ? 1 : 0, nullptr)
    {
    }

    Param::Param(Effect& owner, const char* key, const char* label, core::Select sel)
        : Param(owner, key, label, Kind::Select, 0,
                int16_t(countOptions(sel.options) - 1), sel.def, sel.options)
    {
    }

    bool Param::set(int16_t v)
    {
        v = std::clamp(v, min_, max_);

        // Deliberately a load followed by a store rather than exchange(): the
        // xtensa-lx106 toolchain has no 16-bit atomic read-modify-write and fails
        // to link __atomic_exchange_2. Nothing here needs read-modify-write
        // semantics - the only guarantee a reader needs is a value that is never
        // torn, and plain aligned load/store provides that on both targets.
        const bool changed = value_.load(std::memory_order_relaxed) != v;
        if (changed) value_.store(v, std::memory_order_relaxed);
        return changed;
    }

    void Effect::addParam(Param* p)
    {
        if (paramsTail_ == nullptr) params_ = p;
        else paramsTail_->next_ = p;
        paramsTail_ = p;
    }
}
