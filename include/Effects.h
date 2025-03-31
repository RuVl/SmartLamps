#ifndef EFFECTS_H
#define EFFECTS_H

#include <config.h>
#include <FastLED.h>
#include <GyverDBFile.h>
#include <core/builder.h>

class EffectBase {
public:
    explicit EffectBase(CRGB*, GyverDBFile*);

    virtual ~EffectBase() = default;

    virtual void update() = 0;
    virtual void setSpeed(byte);
    virtual void setBrightness(byte);
    virtual void setScale(byte);

    virtual void buildUI(sets::Builder&);

    [[nodiscard]] byte getSpeed() const;

    [[nodiscard]] byte getBrightness() const;

protected:
    CRGB* leds;
    GyverDBFile* db;

    byte brightness = 50;
    byte speed = 30;
    byte scale = 40;

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
};


class Fire final : public EffectBase {
public:
    explicit Fire(CRGB*, GyverDBFile*);

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
        fire_sparkles
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
        rainbow_hue
    );
};

#endif //EFFECTS_H
