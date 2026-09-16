#pragma once
// Persistent settings.
//
// Backed by GyverDB over LittleFS on both boards. Writes are deferred: the
// value changes in memory immediately and reaches flash once nothing has
// moved for a while, so dragging a slider costs one erase cycle, not fifty.

#include <stdint.h>

namespace hal
{
    class Storage
    {
    public:
        virtual ~Storage() = default;

        virtual void begin() = 0;

        virtual int32_t getInt(uint32_t key, int32_t fallback) = 0;

        virtual void setInt(uint32_t key, int32_t value) = 0;

        // Creates the key with `value` if it does not exist yet. The web panel
        // binds widgets to keys, so a key has to exist before it can be shown.
        virtual void initInt(uint32_t key, int32_t value) = 0;

        virtual const char* getString(uint32_t key, const char* fallback) = 0;

        virtual void setString(uint32_t key, const char* value) = 0;

        // Called every loop; performs the deferred write when due.
        virtual void tick() = 0;

        // Forces the pending write out now. Used before a reconnect or a reboot.
        virtual void flush() = 0;
    };

    Storage& storage();

    // Stable key for a parameter of an effect: hash("<effect>.<param>").
    // Keys of different effects never collide, so a parameter named "speed" can
    // exist in every effect without a prefix.
    inline uint32_t paramKey(const char* effectName, const char* paramKey)
    {
        uint32_t h = 2166136261u;
        auto mix = [&h](const char* s)
        {
            while (*s)
            {
                h ^= uint8_t(*s++);
                h *= 16777619u;
            }
        };
        mix(effectName);
        h ^= uint8_t('.');
        h *= 16777619u;
        mix(paramKey);
        return h;
    }
}
