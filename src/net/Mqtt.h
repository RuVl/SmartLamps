#pragma once

#include <stdint.h>

namespace net::mqtt {
    void begin();

    void tick(uint32_t nowMs);

    void reconnect();

    bool connected();

    bool configured();

    // Publishes the retained JSON state. Called whenever the lamp changed.
    void publishState();
} // namespace net::mqtt
