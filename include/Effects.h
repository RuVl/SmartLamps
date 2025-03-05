#ifndef EFFECTS_H
#define EFFECTS_H

#include <FastLED.h>
#include <GyverTimer.h>

class EffectBase {
public:
    explicit EffectBase(CRGB*);
    virtual ~EffectBase() = default;

    virtual void update() = 0;
    virtual void setSpeed(byte);
    virtual void setBrightness(byte);
    virtual void setScale(byte);

    [[nodiscard]] byte getSpeed() const;
    [[nodiscard]] byte getBrightness() const;

protected:
    CRGB* leds;
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
    explicit SparklesEffect(CRGB*);
    void update() override;
};

class FireEffect final : public EffectBase {
public:
    explicit FireEffect(CRGB*, bool = true);
    void update() override;

private:
    bool sparkles;
    unsigned char* line;
    int pcnt = 0;

    void generateLine() const;
    void shiftUp() const;
    void drawFrame(int pcnt) const;
};

class RainbowEffect final : public EffectBase {
public:
    enum Orientation {
        Vertical,
        Horizontal,
    };

    explicit RainbowEffect(CRGB*, Orientation=Vertical);
    void update() override;

    Orientation orientation;

private:
    byte hue = 0;

    void rainbowVertical();
    void rainbowHorizontal();
};

class TestEffect final : public EffectBase {
public:
    explicit TestEffect(CRGB*);
    void update() override;
};

#endif //EFFECTS_H
