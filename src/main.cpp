#include <Arduino.h>
#include <coredecls.h>

#include "app/Lamp.h"
#include "net/Net.h"

namespace
{
#ifdef LED_DATA_INVERTED
    constexpr int kLedInverted = 1;
#else
    constexpr int kLedInverted = 0;
#endif
#ifdef NPB_CONF_4STEP_CADENCE
    constexpr int kLed4Step = 1;
#else
    constexpr int kLed4Step = 0;
#endif
#ifdef LED_LEAD_PIXELS
    constexpr int kLedLeadPixels = LED_LEAD_PIXELS;
#else
    constexpr int kLedLeadPixels = 0;
#endif
}

void setup()
{
    // By default the core carves the 4 KB loop() stack out of the SDK's 5 KB sys
    // stack ("extra 4K heap"), leaving lwIP callbacks about 1 KB. The panel
    // builds its whole page inside such a callback, and the crash dump showed sp
    // 48 bytes above the sys-stack floor. This no-op call makes the linker put
    // the loop() stack back on the heap: 4 KB less heap, a full sys stack.
    disable_extra4k_at_link_time();
    Serial.begin(SERIAL_BAUD);
    // After a crash this is the only trace left if the exception dump was not captured.
    Serial.println(String(F("reset: ")) + ESP.getResetInfo());
    // The data line's electrical setup, so a log alone tells which build is on the board.
    Serial.printf("led: pin %d inverted=%d 4step=%d lead=%d\n", LED_DATA_PIN,
                  kLedInverted, kLed4Step, kLedLeadPixels);
    app::lamp().begin();
    Serial.printf("state: power=%d brightness=%u effect=%s\n",
                  app::lamp().isOn(), app::lamp().brightness(), app::lamp().effectName());
    net::begin();
}

void loop()
{
    const uint32_t now = millis();
    app::lamp().tick(now);
    net::tick(now);
    app::lamp().render(now);
}
