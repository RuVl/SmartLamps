#ifndef EFFECTS_H
#define EFFECTS_H

#include "config.h"

#include <FastLED.h>
#include <GyverDBFile.h>
#include <core/builder.h>

class EffectBase {
public:
    explicit EffectBase(CRGB*, GyverDBFile*);

    virtual ~EffectBase() = default;

    virtual void update() = 0;
    virtual void buildUI(sets::Builder&);

    [[nodiscard]] byte getSpeed() const;

protected:
    CRGB* leds;
    GyverDBFile* db;

    byte speed = 30;

    void fillAll(CRGB color) const;
    void drawPixelXY(byte x, byte y, CRGB color) const;
    [[nodiscard]] uint32_t getPixColor(int thisSegm) const;
    [[nodiscard]] uint32_t getPixColorXY(byte x, byte y) const;
    static uint16_t getPixelNumber(byte x, byte y);

    void fader(byte step) const;
    void fadePixel(byte i, byte j, byte step) const;
};


class SparklesEffect final : public EffectBase {
public:
    explicit SparklesEffect(CRGB*, GyverDBFile*);

    void update() override;
    void buildUI(sets::Builder&) override;

private:
    static int8_t getFade(byte, byte);

protected:
    DB_KEYS(
        kk,
        sparkles_particles_count,
        sparkles_fade
    );
};


class FireEffect final : public EffectBase {
public:
    explicit FireEffect(CRGB*, GyverDBFile*);

    void update() override;
    void buildUI(sets::Builder&) override;

private:
    unsigned char* line;
    int pcnt = 0;

    void generateLine() const;
    void shiftUp() const;
    void drawFrame(int pcnt) const;

protected:
    DB_KEYS(
        kk,
        fire_sparkles,
        fire_hue
    );
};


class RainbowEffect final : public EffectBase {
public:
    enum Orientation : byte {
        Vertical,
        Horizontal,
    };

    explicit RainbowEffect(CRGB*, GyverDBFile*);

    void update() override;
    void buildUI(sets::Builder&) override;

private:
    byte hue = 0;

    void rainbowVertical();
    void rainbowHorizontal();

protected:
    DB_KEYS(
        kk,
        rainbow_orientation,
        rainbow_speed
    );
};

#endif //EFFECTS_H
