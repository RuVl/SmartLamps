#pragma once
// The WPA2 pre-shared key, derived here instead of by the SDK.
//
// The SDK turns the passphrase into the 256-bit PSK on every association:
// PBKDF2, 4096 rounds of HMAC-SHA1, done in the sys context on the ESP8266,
// where it holds loop() - and the matrix - for close to a second. Given the
// key as 64 hex digits the SDK skips that. So the key is computed once, a
// slice per tick so the frames keep coming, and cached by the caller.
//
// On the ESP32 the WiFi task runs on the other core and the passphrase is
// passed through unchanged.

#include <Arduino.h>

namespace net::psk
{
    // A stable id of the (ssid, passphrase) pair, for the cache.
    uint32_t id(const String& ssid, const String& pass);

    void start(const String& ssid, const String& pass);
    bool busy();
    // One slice of work, a few milliseconds. True once, when key() is ready.
    bool step();
    // 64 hex digits (or the passphrase itself where nothing is derived).
    const String& key();
}
