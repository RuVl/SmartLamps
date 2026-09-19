#pragma once
// A one-slot mailbox between an asynchronous callback and loop().
//
// The SDK reports WiFi drops, MQTT closes and the like from its own context,
// which is no place to build Strings or touch the log. The callback set()s the
// code here and tick() take()s it, so the log line is written from loop().
//
// Plain load/store only, never exchange: xtensa-lx106 has no __atomic_exchange_1
// and would fail to link. A single-word store is atomic on both targets, and the
// release/acquire pair keeps the value visible before the flag on the ESP32's
// second core. The producer may overwrite an unread value; the consumer then
// sees only the latest one, which for a "why did it drop" code is the right one.

#include <atomic>

namespace hal
{
    template <typename T>
    class Mailbox
    {
    public:
        void set(T v)
        {
            value_.store(v, std::memory_order_relaxed);
            dirty_.store(true, std::memory_order_release);
        }

        // True once per set(); v receives the latest value. The flag is cleared
        // before the value is read, so a set() landing in between is not lost -
        // the next take() sees it again, at worst as a duplicate.
        bool take(T& v)
        {
            if (!dirty_.load(std::memory_order_acquire)) return false;
            dirty_.store(false, std::memory_order_relaxed);
            v = value_.load(std::memory_order_acquire);
            return true;
        }

    private:
        std::atomic<T> value_{};
        std::atomic<bool> dirty_{false};
    };
}
