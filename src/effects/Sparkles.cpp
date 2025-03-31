#include "Effects.h"

#include <FastLED.h>

#include "config.h"

SparklesEffect::SparklesEffect(CRGB* _leds, GyverDBFile* _db): EffectBase(_leds, _db) {}

void SparklesEffect::update() {
    for (byte i = 0; i < scale; i++) {
        const byte x = random(0, WIDTH);
        const byte y = random(0, HEIGHT);

        if (getPixColorXY(x, y) == 0)
            leds[getPixelNumber(x, y)] = CHSV(random(0, 255), 255, 255);
    }
    fader(70);
}
