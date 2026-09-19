#include "MemLog.h"

#if defined(LAMP_MEMLOG) && defined(ESP8266)

#include <Arduino.h>

#include "Mqtt.h"
#include "WebUi.h"
#include "app/Lamp.h"

namespace net::memlog
{
    uint32_t g_buildSpLow = UINT32_MAX;

    namespace
    {
        uint32_t g_lastMs = 0;
        uint32_t g_minFree = UINT32_MAX;
        uint32_t g_minBlock = UINT32_MAX;
        uint32_t g_minCont = UINT32_MAX;
    }

    void tick(uint32_t nowMs)
    {
        uint32_t free = 0, block = 0;
        uint8_t frag = 0;
        ESP.getHeapStats(&free, &block, &frag);
        const uint32_t cont = ESP.getFreeContStack();
        if (free < g_minFree) g_minFree = free;
        if (block < g_minBlock) g_minBlock = block;
        if (cont < g_minCont) g_minCont = cont;
        if (nowMs - g_lastMs < 1000) return;
        g_lastMs = nowMs;
        // sys stack grows down from the top of DRAM; depth = how far the
        // build callback got below it.
        const uint32_t sysDepth = g_buildSpLow == UINT32_MAX ? 0 : 0x40000000u - g_buildSpLow;
        Serial.printf("mem: free=%u blk=%u frag=%u cont=%u minfree=%u minblk=%u mincont=%u sys=%u fx=%s foc=%d mqtt=%d up=%u\n",
                      free, block, frag, cont, g_minFree, g_minBlock, g_minCont, sysDepth,
                      app::lamp().effectName(), web::focused() ? 1 : 0, mqtt::connected() ? 1 : 0,
                      nowMs / 1000);
    }
}

#endif
