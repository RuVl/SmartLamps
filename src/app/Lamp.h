#pragma once
// The lamp itself: what is on the matrix, how bright, and what the button did.
//
// This is the only place that knows about all three of core, hal and the
// effect catalogue. Effects stay ignorant of it.

#include <stdint.h>

#include "core/Frame.h"
#include "core/Matrix.h"
#include "core/Registry.h"
#include "hal/Button.h"

namespace app {

class Lamp {
public:
    void begin();

    // Drives everything that is not rendering: button, storage, state.
    void tick(uint32_t nowMs);

    // Renders one frame if one is due. Separate from tick() so the ESP32 can
    // run it on its own core while tick() keeps the network fed.
    void render(uint32_t nowMs);

    void setPower(bool on);
    void togglePower() { setPower(!on_); }
    void setBrightness(uint8_t percent);
    uint8_t brightness() const { return brightnessPercent_; }

    void selectEffect(uint16_t index);
    void nextEffect();
    void prevEffect();

    uint16_t fps() const { return fps_; }

private:
    enum class Transition : uint8_t { None, FadingOut, FadingIn };

    core::Frame frameOf();
    void activate(core::EffectInfo* info);
    void loadParams(core::EffectInfo* info);
    void handle(hal::Gesture g);
    void applyTransition(uint16_t dtMs, uint8_t& brightnessOut);

    static constexpr uint16_t kPixelCount = MATRIX_WIDTH * MATRIX_HEIGHT;
    static constexpr uint16_t kFrameIntervalMs = 1000 / 60;
    static constexpr uint16_t kTransitionMs = 300;

    CRGB pixels_[kPixelCount] = {};
    uint16_t indexMap_[kPixelCount] = {};
    core::Geometry geometry_{};

    // The active effect is built here, never on the heap.
    alignas(8) uint8_t arena_[EFFECT_ARENA_SIZE] = {};
    core::Effect* effect_ = nullptr;
    core::EffectInfo* info_ = nullptr;
    core::EffectInfo* pending_ = nullptr;
    uint16_t effectIndex_ = 0;

    bool on_ = true;
    uint8_t brightnessPercent_ = 50;
    uint8_t brightnessScaled_ = 128;

    Transition transition_ = Transition::None;
    uint16_t transitionMs_ = 0;

    uint32_t lastRenderMs_ = 0;
    uint32_t fpsWindowMs_ = 0;
    uint16_t fpsCounter_ = 0;
    uint16_t fps_ = 0;
};

Lamp& lamp();

}  // namespace app
