#include "Effects.h"

#include <FastLED.h>

#include "config.h"

SparklesEffect::SparklesEffect(CRGB* _leds, GyverDBFile* _db): EffectBase(_leds, _db) {
    _db->init(sparkles_particles_count, 50);
    _db->init(sparkles_fade, 10);
}

void SparklesEffect::update() {
    for (size_t i = 0; i < (size_t)db->get(sparkles_particles_count); i++) {
        const int coord = random(WIDTH * HEIGHT);
        if (!getPixColor(coord) || !random(3))
            leds[coord] = CHSV(random(256), 200, 255);
    }

    const int8_t fade = getFade(db->get(sparkles_particles_count), db->get(sparkles_fade));
    fadeToBlackBy(leds, WIDTH * HEIGHT, fade);
}

int8_t SparklesEffect::getFade(const byte max, const byte min) {
    // 1 < max < 100, 0 < min < 99
    // Interpolate difference like gamma-correction
    return (int8_t)(25 * powf((min + 1.) / max, 0.45) + .5);
}

void SparklesEffect::buildUI(sets::Builder& b) {
    b.Slider2(sparkles_fade, sparkles_particles_count, "Particles",
              0, 100, 1, "", nullptr, nullptr, sets::Colors::Pink);
}
