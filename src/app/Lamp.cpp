#include "Lamp.h"

#include <string.h>

#include "Keys.h"
#include "core/PostFX.h"
#include "hal/LedDriver.h"
#include "hal/Storage.h"

namespace app
{
    namespace
    {
        uint16_t indexOf(const core::EffectInfo* target)
        {
            uint16_t i = 0;
            for (core::EffectInfo* e = core::Registry::head(); e != nullptr; e = e->next, ++i)
                if (e == target) return i;
            return 0;
        }
    }

    void Lamp::begin()
    {
        geometry_ = core::Geometry{
            MATRIX_WIDTH, MATRIX_HEIGHT,
            core::MatrixType::Serpentine,
            core::Corner::BottomLeft,
            core::Direction::Right
        };
        core::buildIndexMap(indexMap_, geometry_);

        // The strip first: it still shows whatever it latched before the reset
        // and the bootloader's traffic on the data pin, and mounting the
        // filesystem takes long enough for that to be visible.
        hal::ledDriver().begin(pixels_, kPixelCount);
        hal::storage().begin();
        hal::button().begin();

        hal::storage().initInt(app::keys::kPower, 1);
        hal::storage().initInt(app::keys::kBrightness, 50);
        hal::storage().initInt(app::keys::kEffect, 0);
        wantOn_ = hal::storage().getInt(app::keys::kPower, 1) != 0;
        setBrightness(uint8_t(hal::storage().getInt(app::keys::kBrightness, 50)));

        const int32_t saved = hal::storage().getInt(app::keys::kEffect, 0);
        selectEffect(uint16_t(saved < 0 ? 0 : saved));

        // No render task exists yet, so the render side is set up directly: the
        // lamp comes back exactly as it was left and fades in without waiting
        // for WiFi.
        activate(wantEffect_);
        litOn_ = wantOn_;
        transition_ = Transition::FadingIn;
        transitionMs_ = 0;
        changed_ = true;
    }

    void Lamp::tick(uint32_t nowMs)
    {
        handle(hal::button().poll(), nowMs);
        hal::storage().tick();
    }

    void Lamp::render(uint32_t nowMs)
    {
        const uint32_t elapsed = nowMs - lastRenderMs_;
        if (elapsed < kFrameIntervalMs) return;
        lastRenderMs_ = nowMs;

        const uint16_t dtMs = uint16_t(elapsed > 1000 ? 1000 : elapsed);
        core::Frame frame = frameOf();

        syncRequests();

        // Keep drawing while fading out, so switching off is a fade, not a cut.
        const bool drawing = litOn_ || transition_ == Transition::FadingOut;
        if (drawing && effect_ != nullptr) effect_->render(frame, dtMs);
        else frame.clear();

        uint8_t brightness = litOn_ ? brightnessScaled_.load(std::memory_order_relaxed) : 0;
        applyTransition(dtMs, brightness);

        if (status() != Status::Ok)
        {
            drawStatus(frame, nowMs);
            // A dark lamp still has to show that it is waiting for WiFi.
            if (brightness < kStatusMinBrightness) brightness = kStatusMinBrightness;
        }

        // Effects draw at full range; the supply limit is applied here, once, to the finished frame.
        brightness = core::limitBrightness(pixels_, kPixelCount, brightness, currentLimitMa_);
        hal::ledDriver().show(brightness);

        ++fpsCounter_;
        if (nowMs - fpsWindowMs_ >= 1000)
        {
            fps_ = fpsCounter_;
            fpsCounter_ = 0;
            fpsWindowMs_ = nowMs;
        }
    }

    core::Frame Lamp::frameOf()
    {
        return core::Frame(pixels_, indexMap_, geometry_);
    }

    void Lamp::drawStatus(core::Frame& f, uint32_t nowMs)
    {
        // Top-left pixel, breathing slowly so it reads as "state", not "stuck".
        // The eye is logarithmic: a linear 96..255 swing looks like a steady
        // light, so the wave goes through a gamma curve and nearly to black.
        const uint8_t phase = uint8_t((nowMs / 8) & 0xFF);
        const uint8_t breath = uint8_t(8 + scale8(dim8_video(cubicwave8(phase)), 247));

        CRGB c;
        switch (status())
        {
        case Status::AccessPoint: c = CRGB(0, 0, breath);
            break; // blue
        case Status::NoWifi: c = CRGB(breath, uint8_t(breath / 3), 0);
            break; // amber
        case Status::NoBroker: c = CRGB(breath, 0, breath);
            break; // magenta
        case Status::Updating: c = CRGB(breath, breath, breath);
            break; // white
        default: return;
        }
        f.at(0, uint8_t(f.height() - 1)) = c;
    }

    void Lamp::syncRequests()
    {
        const bool wantOn = wantOn_.load(std::memory_order_relaxed);
        if (wantOn != litOn_)
        {
            litOn_ = wantOn;
            transition_ = wantOn ? Transition::FadingIn : Transition::FadingOut;
            transitionMs_ = 0;
        }

        core::EffectInfo* wantEffect = wantEffect_.load(std::memory_order_relaxed);
        if (wantEffect == nullptr || wantEffect == info_ || wantEffect == pending_) return;

        // A dark lamp changes effect silently; a lit one fades out first and
        // activate()s at the bottom of the fade. A second request during the
        // fade-out just replaces pending_ - the fade is not restarted - and a
        // switch-off during it keeps pending_: the fade ends in darkness with
        // the new effect built, ready for the next switch-on.
        if (!litOn_ && transition_ == Transition::None)
        {
            activate(wantEffect);
            return;
        }
        pending_ = wantEffect;
        if (transition_ != Transition::FadingOut)
        {
            transition_ = Transition::FadingOut;
            transitionMs_ = 0;
        }
    }

    void Lamp::applyTransition(uint16_t dtMs, uint8_t& brightnessOut)
    {
        if (transition_ == Transition::None) return;

        transitionMs_ = uint16_t(transitionMs_ + dtMs);
        const uint16_t clamped = transitionMs_ > kTransitionMs ? kTransitionMs : transitionMs_;
        const uint8_t progress = uint8_t((uint32_t(clamped) * 255) / kTransitionMs);

        if (transition_ == Transition::FadingOut)
        {
            // dim8_video squares the factor: linear in light is not linear to
            // the eye, which sees a plain 255→0 ramp as "nothing, then a cut".
            const uint8_t full = brightnessScaled_.load(std::memory_order_relaxed);
            brightnessOut = scale8(full, dim8_video(uint8_t(255 - progress)));
            if (transitionMs_ >= kTransitionMs)
            {
                // Only now is the old effect destroyed and the new one built, so
                // a single arena is enough for a visually clean change.
                if (pending_ != nullptr)
                {
                    activate(pending_);
                    pending_ = nullptr;
                }
                // Fade back in unless the lamp was switched off meanwhile.
                transition_ = litOn_ ? Transition::FadingIn : Transition::None;
                transitionMs_ = 0;
            }
        }
        else
        {
            brightnessOut = scale8(brightnessOut, dim8_video(progress));
            if (transitionMs_ >= kTransitionMs) transition_ = Transition::None;
        }
    }

    void Lamp::activate(core::EffectInfo* info)
    {
        if (info == nullptr) return;

        // effect_ is cleared first and published last, so the network side
        // finds either the old list, the new one, or none - see Lamp.h.
        core::Effect* old = effect_;
        effect_ = nullptr;
        if (old != nullptr) old->~Effect();

        info_ = info;
        core::Effect* fresh = info->construct(arena_);
        for (core::Param* p = fresh->params(); p != nullptr; p = p->next())
        {
            const uint32_t key = hal::paramKey(info->name, p->key());
            hal::storage().initInt(key, p->def());
            p->set(int16_t(hal::storage().getInt(key, p->def())));
        }

        core::Frame f = frameOf();
        f.clear();
        fresh->begin(f);
        effect_ = fresh;
        activated_ = true;
    }

    bool Lamp::setParam(const char* key, int16_t value)
    {
        if (effect_ == nullptr || info_ == nullptr) return false;
        for (core::Param* p = effect_->params(); p != nullptr; p = p->next())
        {
            if (strcmp(p->key(), key) != 0) continue;
            if (p->set(value))
            {
                hal::storage().setInt(hal::paramKey(info_->name, key), p->get());
                markChanged();
            }
            return true;
        }
        return false;
    }

    void Lamp::selectEffect(uint16_t index)
    {
        const uint16_t total = core::Registry::count();
        if (total == 0) return;

        effectIndex_ = uint16_t(index % total);
        hal::storage().setInt(app::keys::kEffect, effectIndex_);
        markChanged();
        // render() notices on its next frame and fades over to it.
        wantEffect_.store(core::Registry::at(effectIndex_), std::memory_order_relaxed);
    }

    bool Lamp::selectEffect(const char* name)
    {
        core::EffectInfo* info = core::Registry::find(name);
        if (info == nullptr) return false;
        selectEffect(indexOf(info));
        return true;
    }

    void Lamp::nextEffect() { selectEffect(uint16_t(effectIndex_ + 1)); }

    void Lamp::prevEffect()
    {
        const uint16_t total = core::Registry::count();
        if (total == 0) return;
        selectEffect(uint16_t(effectIndex_ + total - 1));
    }

    void Lamp::setPower(bool on)
    {
        if (isOn() == on) return;
        wantOn_.store(on, std::memory_order_relaxed);
        hal::storage().setInt(app::keys::kPower, on ? 1 : 0);
        markChanged();
    }

    void Lamp::setBrightness(uint8_t percent)
    {
        percent = percent > 100 ? 100 : percent;
        if (percent == brightnessPercent_) return;
        brightnessPercent_ = percent;
        brightnessScaled_.store(core::gammaCorrect(brightnessPercent_), std::memory_order_relaxed);
        hal::storage().setInt(app::keys::kBrightness, brightnessPercent_);
        markChanged();
    }

    bool Lamp::consumeChanged()
    {
        const bool was = changed_;
        changed_ = false;
        return was;
    }

    bool Lamp::consumeActivated()
    {
        // Load then store, not exchange (none on xtensa-lx106). An activation
        // landing between the two is at least 300 ms after the previous one.
        const bool was = activated_.load(std::memory_order_acquire);
        if (was) activated_.store(false, std::memory_order_relaxed);
        return was;
    }

    void Lamp::handle(hal::Gesture g, uint32_t nowMs)
    {
        switch (g)
        {
        // The most frequent action gets the simplest gesture; the previous
        // effect is a panel/MQTT affair.
        case hal::Gesture::Click: togglePower();
            break;
        case hal::Gesture::DoubleClick: nextEffect();
            break;
        case hal::Gesture::TripleClick: break; // ping - lands with the pairing phase (docs/mqtt.md)
        case hal::Gesture::HoldStart:
            // Each hold sweeps the opposite way from the previous one, so
            // dimming never means going all the way up first.
            holdDirection_ = int8_t(-holdDirection_);
            lastHoldStepMs_ = nowMs;
            break;
        case hal::Gesture::HoldTick:
            {
                // poll() reports HoldTick on every loop; the sweep is paced here.
                if (nowMs - lastHoldStepMs_ < kHoldStepMs) break;
                lastHoldStepMs_ = nowMs;
                int next = brightnessPercent_ + holdDirection_;
                if (next > 100) next = 100;
                if (next < 1) next = 1;
                setBrightness(uint8_t(next));
                break;
            }
        case hal::Gesture::HoldEnd: hal::storage().flush();
            break;
        default: break;
        }
    }

    Lamp& lamp()
    {
        static Lamp instance;
        return instance;
    }
}
