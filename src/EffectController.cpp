#include "EffectController.h"

#include "config.h"

CEffectController::CEffectController() {
    leds = new CRGB[NUM_LEDS];

#ifdef LED_MAX_AMPERAGE
    FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_AMPERAGE);
#endif

#ifdef LED_BRIGHTNESS
    FastLED.setBrightness(LED_BRIGHTNESS);
#endif

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS); // NOLINT(*-static-accessed-through-instance)
    FastLED.clear();
}

void CEffectController::changeEffect(EffectBase* _effect) {
    effect = _effect;
    timer.setInterval(effect->getSpeed());
}

void CEffectController::tick() {
    if (effect == nullptr) return;
    if (!timer.isReady()) return;
    effect->update();
    FastLED.show();
}

// externed
CEffectController EffectController;