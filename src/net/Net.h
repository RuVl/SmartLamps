#pragma once
// The network layer as one unit: WiFi, web panel, MQTT, OTA.
//
// Runs on the "other" core on the ESP32 and in the same loop on the ESP8266.
// It drives app::Lamp through its setters and never touches pixels.

#include <stdint.h>

namespace net {

void begin();
void tick(uint32_t nowMs);

}  // namespace net
