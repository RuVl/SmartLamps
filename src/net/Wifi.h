#pragma once

#include <stdint.h>

namespace net::wifi
{
    void begin();

    void tick();

    void reconnect();

    bool connected();

    bool accessPointUp();
}
