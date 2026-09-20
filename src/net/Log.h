#pragma once
// One log for the whole network layer, shown in the web panel and on Serial.

#include <Arduino.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <core/logger.h>
#pragma GCC diagnostic pop

namespace net
{
    // Everything, in order - the "Система" page.
    sets::Logger& log();

    // The lines that start with "WiFi:" / "OTA:" and "MQTT:", for the page
    // where those settings are typed, so the answer to "why not" is next to
    // the credentials. Short: a few lines, not a second journal.
    sets::Logger& wifiLog();
    sets::Logger& mqttLog();

    // Appends a line with the level prefix the panel colours by.
    void logInfo(const String& s);

    void logWarn(const String& s);

    void logError(const String& s);
}
