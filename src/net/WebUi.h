#pragma once

namespace net::web {
    void begin();

    void tick();

    // Pushes the lamp's current state into open panels without a page reload.
    void pushState();

    void pushLog();
} // namespace net::web
