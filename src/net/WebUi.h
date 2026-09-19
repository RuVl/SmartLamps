#pragma once

#include <stdint.h>

namespace net::web
{
    void begin();
    void tick();

    // Appends new log lines to open panels, at most once a second. Returns false
    // when throttled, so the caller keeps its dirty flag.
    bool pushLog();

    // A panel asked for something in the last few seconds.
    bool focused();
}
