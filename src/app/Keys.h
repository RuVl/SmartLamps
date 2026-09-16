#pragma once
// Storage keys for the lamp's own state. Shared with the web panel, whose
// widgets bind to these keys directly so that Settings' own, rate-limited
// update channel keeps the panel in sync — no unsolicited pushes.
//
// Effect parameters get their keys from hal::paramKey and never collide.

#include <stdint.h>

namespace app::keys
{
    constexpr uint32_t kPower = 0x6C616D70; // 'lamp'
    constexpr uint32_t kBrightness = 0x62726774; // 'brgt'
    constexpr uint32_t kEffect = 0x65666378; // 'efcx'
}
