#pragma once
// One log for the whole network layer, shown in the web panel and on Serial.

#include <Arduino.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <core/logger.h>
#pragma GCC diagnostic pop

namespace net {
    sets::Logger &log();

    // Appends a line with the level prefix the panel colours by.
    void logInfo(const String &s);

    void logWarn(const String &s);

    void logError(const String &s);
} // namespace net
