#include "Effect.h"

namespace core {

Param::Param(Effect& owner, const char* key, const char* label,
             int16_t min, int16_t max, int16_t def)
    : key_(key), label_(label), min_(min), max_(max), def_(def), value_(def) {
    owner.addParam(this);
}

bool Param::set(int16_t v) {
    if (v < min_) v = min_;
    if (v > max_) v = max_;
    return value_.exchange(v, std::memory_order_relaxed) != v;
}

void Effect::addParam(Param* p) {
    if (paramsTail_ == nullptr) params_ = p;
    else paramsTail_->next_ = p;
    paramsTail_ = p;
}

}  // namespace core
