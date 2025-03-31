#include "Effects.h"

#include <FastLED.h>

#include "config.h"

// **************** НАСТРОЙКА МАТРИЦЫ ****************
#if (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 0)
#define THIS_WIDTH WIDTH
#define THIS_X x
#define THIS_Y y

#elif (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 1)
#define THIS_WIDTH HEIGHT
#define THIS_X y
#define THIS_Y x

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 0)
#define THIS_WIDTH WIDTH
#define THIS_X x
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 3)
#define THIS_WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y x

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 2)
#define THIS_WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 3)
#define THIS_WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y (WIDTH - x - 1)

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 2)
#define THIS_WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y y

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 1)
#define THIS_WIDTH HEIGHT
#define THIS_X y
#define THIS_Y (WIDTH - x - 1)

#else
#define THIS_WIDTH WIDTH
#define THIS_X x
#define THIS_Y y
#pragma message "Wrong matrix parameters! Set to default"

#endif

EffectBase::EffectBase(CRGB* _leds, GyverDBFile* _db): leds(_leds), db(_db) {}

void EffectBase::setSpeed(const byte _speed) {
    speed = _speed;
}

void EffectBase::setBrightness(const byte _brightness) {
    brightness = _brightness;
}

void EffectBase::setScale(const byte _scale) {
    scale = _scale;
}

void EffectBase::buildUI(sets::Builder& b) {}

byte EffectBase::getSpeed() const {
    return speed;
}

byte EffectBase::getBrightness() const {
    return brightness;
}

// служебные функции

// залить все
void EffectBase::fillAll(const CRGB color) const {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
}

// функция отрисовки точки по координатам X Y
void EffectBase::drawPixelXY(const byte x, const byte y, const CRGB color) const {
    if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
    const int thisPixel = getPixelNumber(x, y) * SEGMENTS;
    for (byte i = 0; i < SEGMENTS; i++) {
        leds[thisPixel + i] = color;
    }
}

// функция получения цвета пикселя по его номеру
uint32_t EffectBase::getPixColor(const int thisSegm) const {
    const int thisPixel = thisSegm * SEGMENTS;
    if (thisPixel < 0 || thisPixel > NUM_LEDS - 1) return 0;
    return static_cast<uint32_t>(leds[thisPixel].r) << 16 | static_cast<long>(leds[thisPixel].g) << 8 | static_cast<long>(leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t EffectBase::getPixColorXY(const byte x, const byte y) const {
    return getPixColor(getPixelNumber(x, y));
}

// получить номер пикселя в ленте по координатам
uint16_t EffectBase::getPixelNumber(const byte x, const byte y) {
    if (THIS_Y % 2 == 0 || MATRIX_TYPE) {
        // если чётная строка
        return (THIS_Y * THIS_WIDTH + THIS_X);
    }
    else {
        // если нечётная строка
        return (THIS_Y * THIS_WIDTH + THIS_WIDTH - THIS_X - 1);
    }
}

// функция плавного угасания цвета для всех пикселей
void EffectBase::fader(const byte step) const {
    for (byte i = 0; i < WIDTH; i++) {
        for (byte j = 0; j < HEIGHT; j++) {
            fadePixel(i, j, step);
        }
    }
}

void EffectBase::fadePixel(const byte i, const byte j, const byte step) const {
    // новый фейдер
    const int pixelNum = getPixelNumber(i, j);
    if (getPixColor(pixelNum) == 0) return;

    if (leds[pixelNum].r >= 30 || leds[pixelNum].g >= 30 || leds[pixelNum].b >= 30) {
        leds[pixelNum].fadeToBlackBy(step);
    }
    else {
        leds[pixelNum] = 0;
    }
}