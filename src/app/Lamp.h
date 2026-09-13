#pragma once
// The lamp itself: what is on the matrix, how bright, and what changed.
//
// This is the only place that knows about all three of core, hal and the
// effect catalogue. Effects stay ignorant of it; the network layer drives it
// through the setters and watches it through consumeChanged().

#include <stdint.h>

#include "core/Frame.h"
#include "core/Matrix.h"
#include "core/Registry.h"
#include "hal/Button.h"

namespace app {
    // What the corner pixel tells the owner. Anything but Ok is drawn over the
    // effect so the lamp can be diagnosed without opening the case.
    enum class Status : uint8_t { Ok, AccessPoint, NoWifi, NoBroker, Updating };

    class Lamp {
    public:
        void begin();

        // Drives everything that is not rendering: button, storage, state.
        void tick(uint32_t nowMs);

        // Renders one frame if one is due. Separate from tick() so the ESP32 can
        // run it on its own core while tick() keeps the network fed.
        void render(uint32_t nowMs);

        // --- state, callable from the button, the panel and MQTT ---
        void setPower(bool on);

        void togglePower() { setPower(!on_); }
        [[nodiscard]] bool isOn() const { return on_; }

        void setBrightness(uint8_t percent);

        [[nodiscard]] uint8_t brightness() const { return brightnessPercent_; }

        void selectEffect(uint16_t index);

        bool selectEffect(const char *name);

        void nextEffect();

        void prevEffect();

        [[nodiscard]] uint16_t effectIndex() const { return effectIndex_; }
        [[nodiscard]] const char *effectName() const { return info_ ? info_->name : ""; }
        [[nodiscard]] core::Param *params() const { return effect_ ? effect_->params() : nullptr; }

        // Sets a parameter of the active effect by key. False if there is none.
        bool setParam(const char *key, int16_t value);

        void setStatus(Status s) { status_ = s; }
        [[nodiscard]] Status status() const { return status_; }

        // True once after any state change; the network publishes on it.
        bool consumeChanged();

        [[nodiscard]] uint16_t fps() const { return fps_; }

    private:
        enum class Transition : uint8_t { None, FadingOut, FadingIn };

        core::Frame frameOf();

        void activate(core::EffectInfo *info);

        void loadParams(core::EffectInfo *info);

        void handle(hal::Gesture g, uint32_t nowMs);

        void applyTransition(uint16_t dtMs, uint8_t &brightnessOut);

        void drawStatus(core::Frame &f, uint32_t nowMs);

        void markChanged() { changed_ = true; }

        static constexpr uint16_t kPixelCount = MATRIX_WIDTH * MATRIX_HEIGHT;
        static constexpr uint16_t kFrameIntervalMs = 1000 / 60;
        static constexpr uint16_t kTransitionMs = 300;
        static constexpr uint8_t kStatusMinBrightness = 24;
    // Holding the pad sweeps 0→100 in about three seconds.
    static constexpr uint16_t kHoldStepMs = 30;

        CRGB pixels_[kPixelCount] = {};
        uint16_t indexMap_[kPixelCount] = {};
        core::Geometry geometry_{};

        // The active effect is built here, never on the heap.
        alignas(8) uint8_t arena_[EFFECT_ARENA_SIZE] = {};
        core::Effect *effect_ = nullptr;
        core::EffectInfo *info_ = nullptr;
        core::EffectInfo *pending_ = nullptr;
        uint16_t effectIndex_ = 0;

        bool on_ = true;
        uint8_t brightnessPercent_ = 50;
        uint8_t brightnessScaled_ = 128;
        Status status_ = Status::Ok;
        bool changed_ = false;
        int8_t holdDirection_ = 1;
        uint32_t lastHoldStepMs_ = 0;

        Transition transition_ = Transition::None;
        uint16_t transitionMs_ = 0;

        uint32_t lastRenderMs_ = 0;
        uint32_t fpsWindowMs_ = 0;
        uint16_t fpsCounter_ = 0;
        uint16_t fps_ = 0;
    };

    Lamp &lamp();
} // namespace app
