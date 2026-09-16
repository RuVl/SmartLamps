#include "Lamp.h"

#include <string.h>

#include <Arduino.h>

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

        hal::storage().begin();
        hal::ledDriver().begin(pixels_, kPixelCount);
        hal::button().begin();

        hal::storage().initInt(app::keys::kPower, 1);
        hal::storage().initInt(app::keys::kBrightness, 50);
        hal::storage().initInt(app::keys::kEffect, 0);
        on_ = hal::storage().getInt(app::keys::kPower, 1) != 0;
        setBrightness(uint8_t(hal::storage().getInt(app::keys::kBrightness, 50)));

        const int32_t saved = hal::storage().getInt(app::keys::kEffect, 0);
        selectEffect(uint16_t(saved < 0 ? 0 : saved));

        // The lamp comes back exactly as it was left, without waiting for WiFi.
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

        // Keep drawing while fading out, so switching off is a fade, not a cut.
        const bool drawing = on_ || transition_ == Transition::FadingOut;
        if (drawing && effect_ != nullptr) effect_->render(frame, dtMs);
        else frame.clear();

        uint8_t brightness = on_ ? brightnessScaled_ : 0;
        applyTransition(dtMs, brightness);

        if (status_ != Status::Ok)
        {
            drawStatus(frame, nowMs);
            // A dark lamp still has to show that it is waiting for WiFi.
            if (brightness < kStatusMinBrightness) brightness = kStatusMinBrightness;
        }

        // Effects draw at full range; the supply limit is applied here, once, to
        // the finished frame.
        brightness = core::limitBrightness(pixels_, kPixelCount, brightness,
                                           LED_CURRENT_LIMIT_MA);
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
        switch (status_)
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
            brightnessOut = scale8(brightnessScaled_, dim8_video(uint8_t(255 - progress)));
            if (transitionMs_ >= kTransitionMs)
            {
                // Only now is the old effect destroyed and the new one built, so
                // a single arena is enough for a visually clean change.
                if (pending_ != nullptr)
                {
                    activate(pending_);
                    pending_ = nullptr;
                    transition_ = Transition::FadingIn;
                }
                else
                {
                    transition_ = Transition::None; // faded out to off
                }
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

        if (effect_ != nullptr)
        {
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

    void Lamp::loadParams(core::EffectInfo* info)
    {
        for (core::Param* p = effect_->params(); p != nullptr; p = p->next())
        {
            const uint32_t key = hal::paramKey(info->name, p->key());
            hal::storage().initInt(key, p->def());
            p->set(int16_t(hal::storage().getInt(key, p->def())));
        }
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
        core::EffectInfo* info = core::Registry::at(effectIndex_);
        hal::storage().setInt(app::keys::kEffect, effectIndex_);
        markChanged();

        if (effect_ == nullptr)
        {
            activate(info); // first boot: nothing to fade out of
            return;
        }
        if (info == info_) return;

        pending_ = info;
        transition_ = Transition::FadingOut;
        transitionMs_ = 0;
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
        if (on_ == on) return;
        on_ = on;
        hal::storage().setInt(app::keys::kPower, on ? 1 : 0);
        markChanged();
        transition_ = on ? Transition::FadingIn : Transition::FadingOut;
        transitionMs_ = 0;
        if (!on) pending_ = nullptr; // fading out to darkness, not to an effect
    }

    void Lamp::setBrightness(uint8_t percent)
    {
        percent = percent > 100 ? 100 : percent;
        if (percent == brightnessPercent_) return;
        brightnessPercent_ = percent;
        brightnessScaled_ = core::gammaCorrect(brightnessPercent_);
        hal::storage().setInt(app::keys::kBrightness, brightnessPercent_);
        markChanged();
    }

    bool Lamp::consumeChanged()
    {
        const bool was = changed_;
        changed_ = false;
        return was;
    }

    void Lamp::handle(hal::Gesture g, uint32_t nowMs)
    {
        if (g != hal::Gesture::None && g != hal::Gesture::HoldTick)
            Serial.printf("btn: %s (power=%d brightness=%u%% pwm=%u effect=%u)\n",
                          hal::gestureName(g), on_, brightnessPercent_, brightnessScaled_, effectIndex_);
        switch (g)
        {
        case hal::Gesture::Click: nextEffect();
            break;
        case hal::Gesture::DoubleClick: prevEffect();
            break;
        case hal::Gesture::TripleClick: break; // ping — lands with the pairing phase
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
        case hal::Gesture::LongHold: togglePower();
            break;
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
