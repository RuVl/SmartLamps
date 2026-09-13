// SmartLamp v2.
//
// Nothing happens here beyond starting the lamp: the wiring lives in app/, so
// the entry point never becomes the place where unrelated subsystems meet.

#include <Arduino.h>

#include "app/Lamp.h"

#ifdef LAMP_BOARD_ESP32S3
namespace {

// Rendering gets its own core. Once the network lands, an open web panel and a
// busy MQTT connection will not be able to steal frames from the matrix.
void renderTask(void*) {
    for (;;) {
        app::lamp().render(millis());
        vTaskDelay(1);
    }
}

}  // namespace
#endif

void setup() {
    Serial.begin(SERIAL_BAUD);
    app::lamp().begin();

#ifdef LAMP_BOARD_ESP32S3
    xTaskCreatePinnedToCore(renderTask, "render", 4096, nullptr, 2, nullptr, 1);
#endif
}

void loop() {
    const uint32_t now = millis();
    app::lamp().tick(now);

#ifndef LAMP_BOARD_ESP32S3
    // One core, one loop — same order, no task.
    app::lamp().render(now);
#endif
}
