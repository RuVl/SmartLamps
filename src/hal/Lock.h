#pragma once
// A mutex for the one place that needs it: the effect arena.
//
// On the ESP32-S3 render() rebuilds the arena in its own task while the panel
// callback or the MQTT client may be walking the Param list that lives in it.
// An atomic pointer would not do - a reader already inside the list when the
// arena is rewritten would follow stale links - so the walk and the rebuild
// exclude each other. Recursive, because a walk may set a parameter, which
// takes the lock again. On the ESP8266 and on the host there is one thread of
// control and the lock is nothing.

#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace hal
{
#ifdef ESP32
    class Lock
    {
    public:
        Lock() : m_(xSemaphoreCreateRecursiveMutex()) {}
        void lock() { xSemaphoreTakeRecursive(m_, portMAX_DELAY); }
        void unlock() { xSemaphoreGiveRecursive(m_); }

    private:
        SemaphoreHandle_t m_;
    };
#else
    class Lock
    {
    public:
        void lock() {}
        void unlock() {}
    };
#endif

    class Guard
    {
    public:
        explicit Guard(Lock& l) : l_(l) { l_.lock(); }
        ~Guard() { l_.unlock(); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

    private:
        Lock& l_;
    };
}
