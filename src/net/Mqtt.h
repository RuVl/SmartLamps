#pragma once

#include <Arduino.h>

namespace net::mqtt
{
    void begin();

    void tick(uint32_t nowMs);

    void reconnect();

    bool connected();

    bool configured();

    // One line for the panel: host when connected, otherwise the last reason.
    String status();

    // Publishes the retained JSON state. Called whenever the lamp changed.
    void publishState();
}
