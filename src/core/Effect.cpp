#include "Effect.h"

#include <algorithm>

namespace core {
    Param::Param(Effect &owner, const char *key, const char *label,
                 int16_t min, int16_t max, int16_t def)
        : key_(key), label_(label), min_(min), max_(max), def_(def), value_(def) {
        owner.addParam(this);
    }

    bool Param::set(int16_t v) {
        v = std::clamp(v, min_, max_);

        // Deliberately a load followed by a store rather than exchange(): the
        // xtensa-lx106 toolchain has no 16-bit atomic read-modify-write and fails
        // to link __atomic_exchange_2. Nothing here needs read-modify-write
        // semantics — the only guarantee a reader needs is a value that is never
        // torn, and plain aligned load/store provides that on both targets.
        const bool changed = value_.load(std::memory_order_relaxed) != v;
        if (changed) value_.store(v, std::memory_order_relaxed);
        return changed;
    }

    void Effect::addParam(Param *p) {
        if (paramsTail_ == nullptr) params_ = p;
        else paramsTail_->next_ = p;
        paramsTail_ = p;
    }
} // namespace core
