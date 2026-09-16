// Settings, shared by both boards: GyverDB on LittleFS.
//
// Writes are deferred by the database itself; tick() performs them when due.
// Dragging a brightness slider therefore costs one flash erase, not fifty.

#include "hal/Storage.h"
#include "hal/Database.h"

#include <Arduino.h>
#include <GyverDBFile.h>
#include <LittleFS.h>

namespace hal
{
    namespace
    {
        GyverDBFile db(&LittleFS, "/lamp.db");

        class DbStorage final : public Storage
        {
        public:
            void begin() override
            {
#ifdef ESP32
                LittleFS.begin(true); // format on first boot
#else
                LittleFS.begin();
#endif
                db.begin();
                db.setTimeout(kWriteDelayMs);
            }

            int32_t getInt(uint32_t key, int32_t fallback) override
            {
                if (!db.has(key)) return fallback;
                return db.get(key);
            }

            void setInt(uint32_t key, int32_t value) override { db.set(key, value); }
            void initInt(uint32_t key, int32_t value) override { db.init(key, value); }

            const char* getString(uint32_t key, const char* fallback) override
            {
                if (!db.has(key)) return fallback;
                scratch_ = db.get(key).toString();
                return scratch_.c_str();
            }

            void setString(uint32_t key, const char* value) override { db.set(key, value); }

            void tick() override { db.tick(); }
            void flush() override { db.update(); }

        private:
            // Ten seconds of quiet before anything reaches flash.
            static constexpr uint16_t kWriteDelayMs = 10000;

            String scratch_;
        };
    }

    Storage& storage()
    {
        static DbStorage instance;
        return instance;
    }

    GyverDBFile& database() { return db; }
}
