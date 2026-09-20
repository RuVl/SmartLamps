#pragma once
// Test-bench telemetry, built only with LAMP_MEMLOG (env d1_mini_mem).
//
// One "mem:" line a second on the UART - heap, largest block, the two stacks -
// parsed by tools/stress/serial_capture.py. Without the flag every call here
// is an empty inline and nothing is linked.

#include <stdint.h>

namespace net::memlog
{
#if defined(LAMP_MEMLOG) && defined(ESP8266)
    // From inside a panel build callback: the build runs on the SDK's sys
    // stack, and reading a1 there is the only way to learn what a page costs.
    inline void sampleStack()
    {
        extern uint32_t g_buildSpLow;
        uint32_t sp;
        asm volatile("mov %0, a1" : "=r"(sp));
        if (sp < g_buildSpLow) g_buildSpLow = sp;
    }

    void tick(uint32_t nowMs);
#else
    inline void sampleStack() {}
    inline void tick(uint32_t) {}
#endif
}
