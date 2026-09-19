#pragma once
// The settings database itself, for the one consumer that binds to it
// directly: the web panel, whose widgets read and write GyverDB keys on their
// own. Everything else goes through hal::Storage.
//
// Arduino-only header; nothing in core/ or effects/ may include it.

#include <GyverDBFile.h>

namespace hal
{
    GyverDBFile& database();

    // A string setting, trimmed: what the owner typed, without the space
    // the phone's keyboard appends.
    inline String dbString(size_t key)
    {
        String s = database().get(key).toString();
        s.trim();
        return s;
    }
}
