#pragma once
// The network layer as one unit: WiFi, web panel, MQTT, OTA.
//
// Ticked from loop() after the lamp, before the frame is rendered.
// It drives app::Lamp through its setters and never touches pixels.

#include <stdint.h>

namespace net
{
    void begin();

    void tick(uint32_t nowMs);
}
