#pragma once
// A one-slot mailbox between an asynchronous callback and loop().
//
// The SDK reports WiFi drops, MQTT closes and the like from its own context,
// which is no place to build Strings or touch the log. The callback set()s the
// code here and tick() take()s it, so the log line is written from loop().
//
// Plain load/store only, never exchange: xtensa-lx106 has no __atomic_exchange_1
// and would fail to link. A single-word store is atomic on both targets. The
// fences are Dekker's: set() publishes the value before the flag, and take()
// clears the flag before it reads the value - a StoreLoad pair, which only a
// full fence orders (one memw on xtensa). So a set() that lands between the
// two is never lost: the next take() sees its flag, at worst as a duplicate.
// The producer may overwrite an unread value; the consumer then sees only
// the latest one, which for a "why did it drop" code is the right one.

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
            std::atomic_thread_fence(std::memory_order_seq_cst);
            dirty_.store(true, std::memory_order_relaxed);
        }

        bool take(T& v)
        {
            if (!dirty_.load(std::memory_order_relaxed)) return false;
            dirty_.store(false, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            v = value_.load(std::memory_order_relaxed);
            return true;
        }

    private:
        std::atomic<T> value_{};
        std::atomic<bool> dirty_{false};
    };
}
