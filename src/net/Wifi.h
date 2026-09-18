#pragma once

#include <Arduino.h>

namespace net::wifi
{
    void begin();

    void tick();

    void reconnect();

    bool connected();

    bool accessPointUp();

    // One line for the panel: IP and RSSI when connected, otherwise what
    // is going on and the last reason the router gave.
    String status();
}
