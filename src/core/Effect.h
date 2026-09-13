#pragma once
// Base class for everything that draws on the matrix.
//
// An effect knows about Frame and its own Params. It does not know about the
// LED driver, the database, the network or Arduino — which is why effects are
// testable on the host and identical on both boards.

#include <stdint.h>

#include "Frame.h"
#include "Param.h"

namespace core {

class Effect {
public:
    virtual ~Effect() = default;

    // Called once after construction, before the first render.
    virtual void begin(Frame& frame) { (void)frame; }

    // Draws one frame. `dtMs` is the time since the previous render, so the
    // animation runs at the same speed whatever the frame rate happens to be.
    // Must not block, allocate, or touch the network.
    virtual void render(Frame& frame, uint16_t dtMs) = 0;

    // Head of the intrusive list of this effect's parameters, in declaration
    // order. Built by Param's constructor.
    [[nodiscard]] Param* params() const { return params_; }

private:
    void addParam(Param* p);

    Param* params_ = nullptr;
    Param* paramsTail_ = nullptr;

    friend class Param;
};

}  // namespace core
