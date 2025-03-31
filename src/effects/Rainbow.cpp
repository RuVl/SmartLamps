#include "Effects.h"

#include <FastLED.h>

#include "config.h"

RainbowEffect::RainbowEffect(CRGB* _leds, GyverDBFile* _db) : EffectBase(_leds, _db) {
    // ReSharper disable once CppRedundantCastExpression
    db->init(rainbow_orientation, (byte)Vertical);
}

void RainbowEffect::update() {
    switch ((Orientation)(byte)db->get(rainbow_orientation)) {
        case Vertical:
            rainbowVertical();
            break;
        case Horizontal:
            rainbowHorizontal();
            break;
    }
}

void RainbowEffect::buildUI(sets::Builder& b) {
    b.Select(rainbow_orientation, "Orientation", F("Vertical;Horizontal"));
}

void RainbowEffect::rainbowVertical() {
    hue += 2;
    for (byte j = 0; j < HEIGHT; j++) {
        auto thisColor = CHSV(hue + j * scale, 255, 255);
        for (byte i = 0; i < WIDTH; i++)
            drawPixelXY(i, j, thisColor);
    }
}

void RainbowEffect::rainbowHorizontal() {
    hue += 2;
    for (byte i = 0; i < WIDTH; i++) {
        auto thisColor = CHSV(hue + i * scale, 255, 255);
        for (byte j = 0; j < HEIGHT; j++)
            drawPixelXY(i, j, thisColor);
    }
}
