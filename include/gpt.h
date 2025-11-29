#ifndef GPT_H
#define GPT_H

#include "config.h"
#include "Effects.h"

#include <FastLED.h>

// === LavaLamp Effect ===
class LavaLamp final : public EffectBase {
public:
    explicit LavaLamp(CRGB* leds, GyverDBFile* db) : EffectBase(leds, db) {}
    void update() override;
};

inline void LavaLamp::update() {
    static uint8_t hue = 0;
    fill_palette(leds, WIDTH * HEIGHT, hue++, 10, LavaColors_p, 255, LINEARBLEND);
}

// === Plasma Effect ===
class Plasma final : public EffectBase {
public:
    explicit Plasma(CRGB* leds, GyverDBFile* db) : EffectBase(leds, db) {}
    void update() override;
};

inline void Plasma::update() {
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (uint8_t y = 0; y < HEIGHT; y++) {
            uint8_t colorIndex = sin8(x * 10 + millis() / 10) + cos8(y * 10 + millis() / 10);
            leds[getPixelNumber(x, y)] = ColorFromPalette(OceanColors_p, colorIndex);
        }
    }
}

// === Confetti Effect ===
class Confetti final : public EffectBase {
public:
    explicit Confetti(CRGB* leds, GyverDBFile* db) : EffectBase(leds, db) {}
    void update() override;
};

inline void Confetti::update() {
    fadeToBlackBy(leds, WIDTH * HEIGHT, 10);
    leds[random(WIDTH * HEIGHT)] += CHSV(random(256), 200, 255);
}

// === Lightning Effect ===
class Lightning final : public EffectBase {
public:
    explicit Lightning(CRGB* leds, GyverDBFile* db) : EffectBase(leds, db) {}
    void update() override;
};

inline void Lightning::update() {
    if (random(100) < 5) {
        fill_solid(leds, WIDTH * HEIGHT, CRGB::White);
    } else {
        fadeToBlackBy(leds, WIDTH * HEIGHT, 40);
    }
}

// === Waterfall Effect ===
class Waterfall final : public EffectBase {
public:
    explicit Waterfall(CRGB* leds, GyverDBFile* db) : EffectBase(leds, db) {}
    void update() override;
};

inline void Waterfall::update() {
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (uint8_t y = HEIGHT - 1; y > 0; y--) {
            leds[getPixelNumber(x, y)] = leds[getPixelNumber(x, y - 1)];
        }
        leds[getPixelNumber(x, 0)] = CHSV(random(160, 200), 255, 255);
    }
}

#endif //GPT_H
