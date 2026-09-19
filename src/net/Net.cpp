#include "Net.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"
#include "Mqtt.h"
#include "Ota.h"
#include "WebUi.h"
#include "Wifi.h"
#include "app/Lamp.h"

namespace net
{
    namespace
    {
        constexpr size_t kLogBytes = 1024;
        sets::Logger g_log(kLogBytes);
        bool g_logDirty = false;

        // A frame is 16 ms; anything that holds loop() much longer freezes the
        // matrix. The line lands right after whatever was logged last, which
        // usually names the culprit. One line per ten seconds is enough.
        constexpr uint32_t kStallMs = 100;
        constexpr uint32_t kStallLogGapMs = 10000;
        uint32_t g_lastLoopMs = 0;
        uint32_t g_lastStallLogMs = 0;

        void watchStall(uint32_t nowMs)
        {
            const uint32_t gap = nowMs - g_lastLoopMs;
            const bool first = g_lastLoopMs == 0;
            g_lastLoopMs = nowMs;
            if (first || gap < kStallMs) return;
            if (g_lastStallLogMs != 0 && nowMs - g_lastStallLogMs < kStallLogGapMs) return;
            g_lastStallLogMs = nowMs;
            logWarn(String(F("цикл стоял ")) + gap + F(" мс"));
        }

        void append(const String& prefix, const String& s)
        {
            g_log.print(prefix);
            g_log.println(s);
            Serial.println(s);
            g_logDirty = true;
        }

#if defined(LAMP_MEMLOG) && defined(ESP8266)
        // Test-bench telemetry (env d1_mini_mem); format parsed by tools/stress/serial_capture.py.
        uint32_t g_memLastMs = 0;
        uint32_t g_minFree = UINT32_MAX;
        uint32_t g_minBlock = UINT32_MAX;
        uint32_t g_minCont = UINT32_MAX;

        void memLog(uint32_t nowMs)
        {
            uint32_t free = 0, block = 0;
            uint8_t frag = 0;
            ESP.getHeapStats(&free, &block, &frag);
            const uint32_t cont = ESP.getFreeContStack();
            if (free < g_minFree) g_minFree = free;
            if (block < g_minBlock) g_minBlock = block;
            if (cont < g_minCont) g_minCont = cont;
            if (nowMs - g_memLastMs < 1000) return;
            g_memLastMs = nowMs;
            // sys stack grows down from the top of DRAM; depth = how far the
            // build callback got below it.
            const uint32_t spLow = web::buildStackLow();
            const uint32_t sysDepth = spLow == UINT32_MAX ? 0 : 0x40000000u - spLow;
            Serial.printf("mem: free=%u blk=%u frag=%u cont=%u minfree=%u minblk=%u mincont=%u sys=%u fx=%s foc=%d mqtt=%d up=%u\n",
                          free, block, frag, cont, g_minFree, g_minBlock, g_minCont, sysDepth,
                          app::lamp().effectName(), web::focused() ? 1 : 0, mqtt::connected() ? 1 : 0,
                          nowMs / 1000);
        }
#else
        inline void memLog(uint32_t) {}
#endif

        // One place decides what the corner pixel shows.
        void updateStatus()
        {
            app::Lamp& lamp = app::lamp();
            if (ota::active()) return; // OTA sets its own status and owns it until reboot

            app::Status s = app::Status::Ok;
            if (!wifi::connected())
            {
                s = wifi::accessPointUp() ? app::Status::AccessPoint : app::Status::NoWifi;
            }
            else if (mqtt::configured() && !mqtt::connected())
            {
                s = app::Status::NoBroker;
            }
            lamp.setStatus(s);
        }
    }

    sets::Logger& log() { return g_log; }
    void logInfo(const String& s) { append(sets::Logger::info(), s); }
    void logWarn(const String& s) { append(sets::Logger::warn(), s); }
    void logError(const String& s) { append(sets::Logger::error(), s); }

    void begin()
    {
        refreshNames();
        logInfo(String(F("Лампа ")) + lampName());

        // Order matters: WiFi sets the radio mode the panel's server needs,
        // and OTA registers its callbacks before WiFi can call start() on it.
        ota::begin();
        wifi::begin();
        web::begin();
        mqtt::begin();
    }

    void tick(uint32_t nowMs)
    {
        watchStall(nowMs);
        wifi::tick();
        web::tick();
        mqtt::tick(nowMs);
        ota::tick();

        updateStatus();
        memLog(nowMs);

        // The panel is not pushed to: its widgets are bound to the database and
        // Settings syncs them itself. MQTT is the only subscriber to changes.
        if (app::lamp().consumeChanged()) mqtt::publishState();
        if (g_logDirty && web::pushLog()) g_logDirty = false;
    }
}
