#pragma once
// The lamp itself: what is on the matrix, how bright, and what changed.
//
// This is the only place that knows about all three of core, hal and the
// effect catalogue. Effects stay ignorant of it; the network layer drives it
// through the setters and watches it through consumeChanged().
//
// On the ESP32-S3 render() runs in its own task while the setters are called
// from loop(), from the panel's WebSocket callback and from the MQTT client -
// there is no lock between them. The setters therefore never touch the
// transition: they store what the owner wants (wantOn_, wantEffect_, single
// atomic words), and render() compares that with what is on the matrix at the
// start of every frame and drives the fade itself. Everything about the
// transition is owned by render() alone.

#include <stdint.h>

#include <atomic>

#include "core/Frame.h"
#include "core/Matrix.h"
#include "core/Registry.h"
#include "hal/Button.h"

namespace app
{
    // What the corner pixel tells the owner. Anything but Ok is drawn over the
    // effect so the lamp can be diagnosed without opening the case.
    enum class Status : uint8_t { Ok, AccessPoint, NoWifi, NoBroker, Updating };

    class Lamp
    {
    public:
        void begin();

        // Drives everything that is not rendering: button, storage, state.
        void tick(uint32_t nowMs);

        // Renders one frame if one is due. Separate from tick() so the ESP32 can
        // run it on its own core while tick() keeps the network fed.
        void render(uint32_t nowMs);

        // --- state, callable from the button, the panel and MQTT ---
        void setPower(bool on);

        void togglePower() { setPower(!isOn()); }
        [[nodiscard]] bool isOn() const { return wantOn_.load(std::memory_order_relaxed); }

        void setBrightness(uint8_t percent);

        [[nodiscard]] uint8_t brightness() const { return brightnessPercent_; }

        void selectEffect(uint16_t index);

        bool selectEffect(const char* name);

        void nextEffect();

        void prevEffect();

        [[nodiscard]] uint16_t effectIndex() const { return effectIndex_; }
        [[nodiscard]] const char* effectName() const { return info_ ? info_->name : ""; }
        [[nodiscard]] core::Param* params() const { return effect_ ? effect_->params() : nullptr; }

        // Sets a parameter of the active effect by key. False if there is none.
        bool setParam(const char* key, int16_t value);

        void setStatus(Status s) { status_.store(s, std::memory_order_relaxed); }
        [[nodiscard]] Status status() const { return status_.load(std::memory_order_relaxed); }

        // True once after any state change; the network publishes on it.
        bool consumeChanged();

        // True once after a new effect has actually been built in the arena.
        // selectEffect() only schedules the change; the panel needs the moment
        // the new Param list exists, which is after the fade-out.
        bool consumeActivated();

        [[nodiscard]] uint16_t fps() const { return fps_.load(std::memory_order_relaxed); }

        // Supply limit in mA, LED_CURRENT_LIMIT_MA by default. The simulator
        // toggles it to show the lamp with and without the limiter.
        void setCurrentLimit(uint16_t mA) { currentLimitMa_ = mA; }
        [[nodiscard]] uint16_t currentLimit() const { return currentLimitMa_; }

        // The strip index of every (x, y), row by row from the bottom-left -
        // for whoever needs to read the pixel buffer back in matrix order.
        [[nodiscard]] const uint16_t* indexMap() const { return indexMap_; }

    private:
        enum class Transition : uint8_t { None, FadingOut, FadingIn };

        core::Frame frameOf();

        void activate(core::EffectInfo* info);

        // Render side: picks up a changed wantOn_/wantEffect_ and starts the fade.
        void syncRequests();

        void handle(hal::Gesture g, uint32_t nowMs);

        void applyTransition(uint16_t dtMs, uint8_t& brightnessOut);

        void drawStatus(core::Frame& f, uint32_t nowMs);

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
        // --- written by the setters, read by render() ---
        // Single-word atomics, load/store only: xtensa-lx106 has no atomic
        // read-modify-write, and none is needed - the latest wish wins.
        std::atomic<bool> wantOn_{true};
        std::atomic<core::EffectInfo*> wantEffect_{nullptr};
        std::atomic<uint8_t> brightnessScaled_{128};
        std::atomic<Status> status_{Status::Ok};
        uint8_t brightnessPercent_ = 50;
        uint16_t effectIndex_ = 0;
        uint16_t currentLimitMa_ = LED_CURRENT_LIMIT_MA;
        bool changed_ = false;
        int8_t holdDirection_ = 1;
        uint32_t lastHoldStepMs_ = 0;

        // --- owned by render() ---
        // The active effect and its Param list live in the arena and are rebuilt
        // by activate(). params() and setParam() read them from the network side
        // without a lock; activate() clears effect_ for the microseconds the
        // arena is being rewritten, so a reader arriving then sees no params. A
        // reader already walking the list at that instant is the residual race:
        // ESP32 only, once per effect change, and a lock around the panel build
        // would cost more frames than it is worth.
        core::Effect* effect_ = nullptr;
        core::EffectInfo* info_ = nullptr;
        core::EffectInfo* pending_ = nullptr; // activated once the fade-out ends
        bool litOn_ = true; // what the matrix shows; follows wantOn_ through a fade
        Transition transition_ = Transition::None;
        uint16_t transitionMs_ = 0;
        std::atomic<bool> activated_{false};
        std::atomic<uint16_t> fps_{0};

        uint32_t lastRenderMs_ = 0;
        uint32_t fpsWindowMs_ = 0;
        uint16_t fpsCounter_ = 0;
    };

    Lamp& lamp();
}
