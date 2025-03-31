#include "EffectController.h"

#include <core/containers.h>

#include "config.h"

PGM_P effectNames PROGMEM =
        "Sparkles;"
        "Fire;"
        "Rainbow;";

EffectControllerClass& EffectControllerClass::instance() {
    static EffectControllerClass instance;
    return instance;
}

EffectControllerClass::EffectControllerClass() {
    leds = new CRGB[NUM_LEDS];
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS); // NOLINT(*-static-accessed-through-instance)

#ifdef LED_MAX_AMPERAGE
    FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_AMPERAGE);
#endif
}

void EffectControllerClass::begin(GyverDBFile& _db) {
    db = &_db;

    // ReSharper disable once CppRedundantCastExpression
    _db.init(active_effect, (byte)lastEffectIndex);
    lastEffectIndex = EFFECT_COUNT;

    setEffect((EffectType)(byte)_db.get(active_effect));
}

bool EffectControllerClass::setEffect(const EffectType type) {
    if (lastEffectIndex == type) return false;
    if (currentEffect != nullptr) delete currentEffect;

    FastLED.clear(true); // wipe matrix

    lastEffectIndex = type;
    currentEffect = createEffect(type);

    speedTimer.setInterval(currentEffect != nullptr ? currentEffect->getSpeed() : 0);
    return true;
}

void EffectControllerClass::tick() {
    if (currentEffect == nullptr) return;
    if (!speedTimer.isReady()) return;

    currentEffect->update();
#ifdef LED_BRIGHTNESS
    FastLED.setBrightness(LED_BRIGHTNESS);
#endif
    FastLED.show();
}

void EffectControllerClass::buildUI(sets::Builder& b) {
    {
        sets::Menu _(b, "LED effects");

        if (b.Select(active_effect, "Effect", FPSTR(effectNames)))
            if (setEffect((EffectType)(byte)db->get(active_effect)))
                b.reload();

        if (currentEffect != nullptr) currentEffect->buildUI(b);
    }
}

EffectBase* EffectControllerClass::createEffect(const EffectType type) const {
    switch (type) {
        case SPARKLES: return new SparklesEffect(leds, db);
        case FIRE: return new Fire(leds, db);
        case RAINBOW: return new RainbowEffect(leds, db);
        default: return nullptr;
    }
}
