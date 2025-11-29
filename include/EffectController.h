#ifndef EFFECTCONTROLLER_H
#define EFFECTCONTROLLER_H

#include "config.h"

#include <FastLED.h>
#include <GyverDBFile.h>
#include <GyverTimer.h>
#include <core/builder.h>

#include "Effects.h"
#include "gpt.h"

// При изменении не забудь изменить effectNames
enum EffectType : byte {
    SPARKLES,
    FIRE,
    RAINBOW,

    LAVA_LAMP,
    PLASMA,
    CONFETTI,
    LIGHTNING,
    WATERFALL,
    
    EFFECT_COUNT // Хранит количество эффектов
};

class EffectControllerClass {
public:
    static EffectControllerClass& instance();
    EffectControllerClass(EffectControllerClass const&) = delete;
    void operator=(EffectControllerClass const&) = delete;

    void begin(GyverDBFile&);
    void tick();
    
    bool setEffect(EffectType);
    void buildUI(sets::Builder&);

private:
    EffectControllerClass();

    GyverDBFile* db = nullptr;

    CRGB* leds;
    GTimer speedTimer;

    EffectBase* currentEffect = nullptr;
    EffectType lastEffectIndex = SPARKLES; // default

    byte maxBrightness = 255;
    static byte gammaCorrection(float);
    
protected:
    EffectBase* createEffect(EffectType) const;

    DB_KEYS(
        kk,
        brightness_percent,
        active_effect
    );
};

#define EffectController EffectControllerClass::instance()

#endif //EFFECTCONTROLLER_H
