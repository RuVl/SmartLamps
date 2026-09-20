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
                mounted_ = LittleFS.begin(true); // format on first boot
#else
                mounted_ = LittleFS.begin();
#endif
                if (!mounted_) {
                    // Say it once, loudly. Without this the only symptom is
                    // "File system is not mounted" from vfs_api every time
                    // the deferred write fires — ten seconds apart, forever.
                    Serial.println(F("storage: LittleFS mount FAILED — settings will not persist"));
                    return;
                }
#ifdef ESP32
                Serial.printf("storage: LittleFS %u/%u KB used\n",
                              unsigned(LittleFS.usedBytes() / 1024),
                              unsigned(LittleFS.totalBytes() / 1024));
#else
                FSInfo fi;
                LittleFS.info(fi);
                Serial.printf("storage: LittleFS %u/%u KB used\n",
                              unsigned(fi.usedBytes / 1024), unsigned(fi.totalBytes / 1024));
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

            // With no filesystem the database still works in RAM — the lamp
            // runs on defaults — but nothing is written, so nothing spams.
            void tick() override { if (mounted_) db.tick(); }
            void flush() override { if (mounted_) db.update(); }

        private:
            // Ten seconds of quiet before anything reaches flash.
            static constexpr uint16_t kWriteDelayMs = 10000;

            bool mounted_ = false;
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
