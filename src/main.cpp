#include <Arduino.h>

#include "app/Lamp.h"
#include "net/Net.h"

#ifdef LAMP_BOARD_ESP8266
#include <coredecls.h>

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
#endif

#ifdef LAMP_BOARD_ESP32S3
namespace
{
    // Rendering gets its own core. Once the network lands, an open web panel and a
    // busy MQTT connection will not be able to steal frames from the matrix.
    void renderTask(void*)
    {
        for (;;)
        {
            app::lamp().render(millis());
            vTaskDelay(1);
        }
    }
}
#endif

void setup()
{
#ifdef LAMP_BOARD_ESP8266
    // By default the core carves the 4 KB loop() stack out of the SDK's 5 KB sys
    // stack ("extra 4K heap"), leaving lwIP callbacks about 1 KB. The panel
    // builds its whole page inside such a callback, and the crash dump showed sp
    // 48 bytes above the sys-stack floor. This no-op call makes the linker put
    // the loop() stack back on the heap: 4 KB less heap, a full sys stack.
    disable_extra4k_at_link_time();
#endif
    Serial.begin(SERIAL_BAUD);
#ifdef LAMP_BOARD_ESP8266
    // The data line's electrical setup, so a log alone tells which build is on the board.
    Serial.printf("led: pin %d inverted=%d 4step=%d lead=%d\n", LED_DATA_PIN,
                  kLedInverted, kLed4Step, kLedLeadPixels);
#endif
    app::lamp().begin();
    Serial.printf("state: power=%d brightness=%u effect=%s\n",
                  app::lamp().isOn(), app::lamp().brightness(), app::lamp().effectName());
    net::begin();

#ifdef LAMP_BOARD_ESP32S3
    xTaskCreatePinnedToCore(renderTask, "render", 4096, nullptr, 2, nullptr, 1);
#endif
}

void loop()
{
    const uint32_t now = millis();
    app::lamp().tick(now);
    net::tick(now);

#ifndef LAMP_BOARD_ESP32S3
    // One core, one loop - same order, no task.
    app::lamp().render(now);
#endif
}
