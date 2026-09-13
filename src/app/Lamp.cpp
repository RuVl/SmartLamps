#include "Lamp.h"

#include "core/PostFX.h"
#include "hal/LedDriver.h"
#include "hal/Storage.h"

namespace app {
namespace {

// Storage keys for the lamp's own state. Effect parameters get their keys
// from hal::paramKey and never collide with these.
constexpr uint32_t kKeyPower = 0x6C616D70;       // 'lamp'
constexpr uint32_t kKeyBrightness = 0x62726774;  // 'brgt'
constexpr uint32_t kKeyEffect = 0x65666378;      // 'efcx'

}  // namespace

void Lamp::begin() {
    geometry_ = core::Geometry{MATRIX_WIDTH, MATRIX_HEIGHT,
                               core::MatrixType::Serpentine,
                               core::Corner::BottomLeft,
                               core::Direction::Right};
    core::buildIndexMap(indexMap_, geometry_);

    hal::storage().begin();
    hal::ledDriver().begin(pixels_, kPixelCount);
    hal::button().begin();

    on_ = hal::storage().getInt(kKeyPower, 1) != 0;
    setBrightness(uint8_t(hal::storage().getInt(kKeyBrightness, 50)));

    const int32_t saved = hal::storage().getInt(kKeyEffect, 0);
    selectEffect(uint16_t(saved < 0 ? 0 : saved));

    // The lamp comes back exactly as it was left, without waiting for WiFi.
    transition_ = Transition::FadingIn;
    transitionMs_ = 0;
}

void Lamp::tick(uint32_t nowMs) {
    (void)nowMs;
    handle(hal::button().poll());
    hal::storage().tick();
}

void Lamp::render(uint32_t nowMs) {
    const uint32_t elapsed = nowMs - lastRenderMs_;
    if (elapsed < kFrameIntervalMs) return;
    lastRenderMs_ = nowMs;

    const uint16_t dtMs = uint16_t(elapsed > 1000 ? 1000 : elapsed);

    core::Frame frame = frameOf();
    if (effect_ != nullptr) effect_->render(frame, dtMs);

    uint8_t brightness = on_ ? brightnessScaled_ : 0;
    applyTransition(dtMs, brightness);

    // Effects draw at full range; brightness, gamma and the supply limit are
    // applied here, once, to the finished frame.
    brightness = core::limitBrightness(pixels_, kPixelCount, brightness,
                                       LED_CURRENT_LIMIT_MA);
    hal::ledDriver().show(brightness);

    ++fpsCounter_;
    if (nowMs - fpsWindowMs_ >= 1000) {
        fps_ = fpsCounter_;
        fpsCounter_ = 0;
        fpsWindowMs_ = nowMs;
    }
}

core::Frame Lamp::frameOf() {
    return core::Frame(pixels_, indexMap_, geometry_);
}

void Lamp::applyTransition(uint16_t dtMs, uint8_t& brightnessOut) {
    if (transition_ == Transition::None) return;

    transitionMs_ = uint16_t(transitionMs_ + dtMs);
    const uint16_t clamped = transitionMs_ > kTransitionMs ? kTransitionMs : transitionMs_;
    const uint8_t progress = uint8_t((uint32_t(clamped) * 255) / kTransitionMs);

    if (transition_ == Transition::FadingOut) {
        brightnessOut = scale8(brightnessOut, uint8_t(255 - progress));
        if (transitionMs_ >= kTransitionMs) {
            // Only now is the old effect destroyed and the new one built, so
            // a single arena is enough for a visually clean change.
            activate(pending_);
            pending_ = nullptr;
            transition_ = Transition::FadingIn;
            transitionMs_ = 0;
        }
    } else {
        brightnessOut = scale8(brightnessOut, progress);
        if (transitionMs_ >= kTransitionMs) transition_ = Transition::None;
    }
}

void Lamp::activate(core::EffectInfo* info) {
    if (info == nullptr) return;

    if (effect_ != nullptr) {
        effect_->~Effect();
        effect_ = nullptr;
    }

    info_ = info;
    effect_ = info->construct(arena_);
    loadParams(info);

    core::Frame f = frameOf();
    f.clear();
    effect_->begin(f);
}

void Lamp::loadParams(core::EffectInfo* info) {
    for (core::Param* p = effect_->params(); p != nullptr; p = p->next()) {
        const uint32_t key = hal::paramKey(info->name, p->key());
        p->set(int16_t(hal::storage().getInt(key, p->def())));
    }
}

void Lamp::selectEffect(uint16_t index) {
    const uint16_t total = core::Registry::count();
    if (total == 0) return;

    effectIndex_ = uint16_t(index % total);
    core::EffectInfo* info = core::Registry::at(effectIndex_);
    hal::storage().setInt(kKeyEffect, effectIndex_);

    if (effect_ == nullptr) {
        activate(info);  // first boot: nothing to fade out of
        return;
    }

    pending_ = info;
    transition_ = Transition::FadingOut;
    transitionMs_ = 0;
}

void Lamp::nextEffect() { selectEffect(uint16_t(effectIndex_ + 1)); }

void Lamp::prevEffect() {
    const uint16_t total = core::Registry::count();
    if (total == 0) return;
    selectEffect(uint16_t(effectIndex_ + total - 1));
}

void Lamp::setPower(bool on) {
    if (on_ == on) return;
    on_ = on;
    hal::storage().setInt(kKeyPower, on ? 1 : 0);
    transition_ = on ? Transition::FadingIn : Transition::FadingOut;
    transitionMs_ = 0;
    if (!on) pending_ = nullptr;  // fading out to darkness, not to an effect
}

void Lamp::setBrightness(uint8_t percent) {
    brightnessPercent_ = percent > 100 ? 100 : percent;
    brightnessScaled_ = core::gammaCorrect(brightnessPercent_);
    hal::storage().setInt(kKeyBrightness, brightnessPercent_);
}

void Lamp::handle(hal::Gesture g) {
    switch (g) {
        case hal::Gesture::Click: nextEffect(); break;
        case hal::Gesture::DoubleClick: prevEffect(); break;
        case hal::Gesture::TripleClick: break;  // ping — lands with the MQTT phase
        case hal::Gesture::HoldTick: {
            // Brightness ramps up, wraps around at the top.
            const uint8_t next = uint8_t(brightnessPercent_ >= 100 ? 5 : brightnessPercent_ + 1);
            setBrightness(next);
            break;
        }
        case hal::Gesture::LongHold: togglePower(); break;
        case hal::Gesture::HoldEnd: hal::storage().flush(); break;
        default: break;
    }
}

Lamp& lamp() {
    static Lamp instance;
    return instance;
}

}  // namespace app
