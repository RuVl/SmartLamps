#ifndef EFFECTCONTROLLER_H
#define EFFECTCONTROLLER_H

#include <FastLED.h>
#include <GyverDBFile.h>
#include <GyverTimer.h>
#include <core/builder.h>

#include "Effects.h"

// При изменении не забудь изменить effectNames
enum EffectType : byte {
    SPARKLES,
    FIRE,
    RAINBOW,
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

protected:
    EffectBase* createEffect(EffectType) const;

    DB_KEYS(
        kk,
        active_effect
    );
};

#define EffectController EffectControllerClass::instance()

#endif //EFFECTCONTROLLER_H
