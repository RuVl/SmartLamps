#include "EffectController.h"

#include <core/containers.h>

PGM_P effectNames PROGMEM =
        "Sparkles;"
        "Fire;"
        "Rainbow;"
        "LavaLamp;"
        "Plasma;"
        "Confetti;"
        "Lightning;"
        "Waterfall;";

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
    _db.init(brightness_percent, 50);

    lastEffectIndex = EFFECT_COUNT;
    maxBrightness = gammaCorrection(_db.get(brightness_percent));

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

    FastLED.setBrightness(maxBrightness);
    currentEffect->update();
    FastLED.show();
}

void EffectControllerClass::buildUI(sets::Builder& b) {
    {
        sets::Menu _(b, "LED effects");

        byte b_prc = db->get(brightness_percent);
        if (b.Slider(brightness_percent, "Max brigthness", 0, 100, 1, "%", &b_prc, sets::Colors::Violet)) {
            db->set(brightness_percent, b_prc); // update db manually
            maxBrightness = gammaCorrection(b_prc);
        }

        if (b.Select(active_effect, "Effect", FPSTR(effectNames)))
            if (setEffect((EffectType)(byte)db->get(active_effect)))
                b.reload();

        if (currentEffect != nullptr) currentEffect->buildUI(b);
    }
}

byte EffectControllerClass::gammaCorrection(const float level) {
    if (level == 0) return 0;
    if (level == 100) return 255;

    return (byte)(pow(level / 100.0, GAMMA) * 255. + .5); // round
}

EffectBase* EffectControllerClass::createEffect(const EffectType type) const {
    switch (type) {
        case SPARKLES: return new SparklesEffect(leds, db);
        case FIRE: return new FireEffect(leds, db);
        case RAINBOW: return new RainbowEffect(leds, db);
        case LAVA_LAMP: return new LavaLamp(leds, db);
        case PLASMA: return new Plasma(leds, db);
        case CONFETTI: return new Confetti(leds, db);
        case LIGHTNING: return new Lightning(leds, db);
        case WATERFALL: return new Waterfall(leds, db);
        default: return nullptr;
    }
}
