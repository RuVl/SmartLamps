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

EffectBase::EffectBase(CRGB* _leds): leds(_leds) {}

void EffectBase::setSpeed(const byte _speed) {
    speed = _speed;
}

void EffectBase::setBrightness(const byte _brightness) {
    brightness = _brightness;
}

void EffectBase::setScale(const byte _scale) {
    scale = _scale;
}

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

// ================================= ЭФФЕКТЫ ====================================

// --------------------------------- конфетти ------------------------------------
SparklesEffect::SparklesEffect(CRGB* _leds): EffectBase(_leds) {}

void SparklesEffect::update() {
    for (byte i = 0; i < scale; i++) {
        const byte x = random(0, WIDTH);
        const byte y = random(0, HEIGHT);

        if (getPixColorXY(x, y) == 0)
            leds[getPixelNumber(x, y)] = CHSV(random(0, 255), 255, 255);
    }
    fader(70);
}

// -------------------------------------- огонь ---------------------------------------------
bool loadingFlag = true;
unsigned char matrixValue[8][16];
//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[8][16] PROGMEM = {
    {32, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 32},
    {64, 0, 0, 0, 0, 0, 0, 64, 64, 0, 0, 0, 0, 0, 0, 64},
    {96, 32, 0, 0, 0, 0, 32, 96, 96, 32, 0, 0, 0, 0, 32, 96},
    {128, 64, 32, 0, 0, 32, 64, 128, 128, 64, 32, 0, 0, 32, 64, 128},
    {160, 96, 64, 32, 32, 64, 96, 160, 160, 96, 64, 32, 32, 64, 96, 160},
    {192, 128, 96, 64, 64, 96, 128, 192, 192, 128, 96, 64, 64, 96, 128, 192},
    {255, 160, 128, 96, 96, 128, 160, 255, 255, 160, 128, 96, 96, 128, 160, 255},
    {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char hueMask[8][16] PROGMEM = {
    {1, 11, 19, 25, 25, 22, 11, 1, 1, 11, 19, 25, 25, 22, 11, 1},
    {1, 8, 13, 19, 25, 19, 8, 1, 1, 8, 13, 19, 25, 19, 8, 1},
    {1, 8, 13, 16, 19, 16, 8, 1, 1, 8, 13, 16, 19, 16, 8, 1},
    {1, 5, 11, 13, 13, 13, 5, 1, 1, 5, 11, 13, 13, 13, 5, 1},
    {1, 5, 11, 11, 11, 11, 5, 1, 1, 5, 11, 11, 11, 11, 5, 1},
    {0, 1, 5, 8, 8, 5, 1, 0, 0, 1, 5, 8, 8, 5, 1, 0},
    {0, 0, 1, 5, 5, 1, 0, 0, 0, 0, 1, 5, 5, 1, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0}
};

FireEffect::FireEffect(CRGB* _leds, const bool _sparkles) : EffectBase(_leds) {
    line = new unsigned char[WIDTH];
    sparkles = _sparkles;
    scale = 0;
    memset(matrixValue, 0, sizeof(matrixValue));
}

void FireEffect::update() {
    if (loadingFlag) {
        loadingFlag = false;
        //FastLED.clear();
        generateLine();
    }
    if (pcnt >= 100) {
        shiftUp();
        generateLine();
        pcnt = 0;
    }
    drawFrame(pcnt);
    pcnt += 30;
}

void FireEffect::generateLine() const {
    for (uint8_t x = 0; x < WIDTH; x++) {
        line[x] = random(64, 255);
    }
}

void FireEffect::shiftUp() const {
    for (uint8_t y = HEIGHT - 1; y > 0; y--) {
        for (uint8_t x = 0; x < WIDTH; x++) {
            uint8_t newX = x;
            if (x > 15) newX = x - 15;
            if (y > 7) continue;
            matrixValue[y][newX] = matrixValue[y - 1][newX];
        }
    }

    for (uint8_t x = 0; x < WIDTH; x++) {
        uint8_t newX = x;
        if (x > 15) newX = x - 15;
        matrixValue[0][newX] = line[newX];
    }
}

void FireEffect::drawFrame(const int pcnt) const {
    // Each row interpolates with the one before it
    for (unsigned char y = HEIGHT - 1; y > 0; y--) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            uint8_t newX = x;
            if (x > 15) newX = x - 15;
            if (y < 8) {
                int nextv = static_cast<byte>(((100.0 - pcnt) * matrixValue[y][newX] + pcnt * matrixValue[y - 1][newX]) / 100.0)
                    - pgm_read_byte(&valueMask[y][newX]);

                const CRGB color = CHSV(
                    static_cast<byte>(scale * 2.5) + pgm_read_byte(&hueMask[y][newX]), // H
                    255, // S
                    max(0, nextv) // V
                );

                leds[getPixelNumber(x, y)] = color;
            }
            else if (y == 8 && sparkles) {
                if (random(0, 20) == 0 && getPixColorXY(x, y - 1) != 0) drawPixelXY(x, y, getPixColorXY(x, y - 1));
                else drawPixelXY(x, y, 0);
            }
            else if (sparkles) {
                // старая версия для яркости
                if (getPixColorXY(x, y - 1) > 0) {
                    drawPixelXY(x, y, getPixColorXY(x, y - 1));
                }
                else drawPixelXY(x, y, 0);
            }
        }
    }

    // The first row interpolates with the "next" line
    for (unsigned char x = 0; x < WIDTH; x++) {
        uint8_t newX = x;
        if (x > 15) newX = x - 15;
        const CRGB color = CHSV(
            static_cast<byte>(scale * 2.5) + pgm_read_byte(&hueMask[0][newX]), // H
            255, // S
            static_cast<byte>(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0) // V
        );
        leds[getPixelNumber(newX, 0)] = color;
    }
}

// ---------------------------------------- радуга ------------------------------------------
RainbowEffect::RainbowEffect(CRGB* _leds, const Orientation _orientation) : EffectBase(_leds), orientation(_orientation) {}

void RainbowEffect::update() {
    if (orientation == Vertical) rainbowVertical();
    else rainbowHorizontal();
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

TestEffect::TestEffect(CRGB* _leds) : EffectBase(_leds) {}
void TestEffect::update() {
    fillAll(CRGB::White);
}



