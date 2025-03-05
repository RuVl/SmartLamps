#ifndef EFFECTCONTROLLER_H
#define EFFECTCONTROLLER_H

#include <FastLED.h>
#include "Effects.h"

class CEffectController {
public:
    explicit CEffectController();

    void changeEffect(EffectBase*);
    void tick();

    CRGB* leds;

private:
    EffectBase* effect = nullptr;
    GTimer timer;
};

extern CEffectController EffectController;

#endif //EFFECTCONTROLLER_H
