#pragma once
// Hardware self-test, built with -DLAMP_SELFTEST (env d1_mini_selftest).
//
// Replaces the whole application with two alternating phases so the wiring
// can be checked with nothing more than a multimeter:
//   A — the data pin toggles at 1 Hz: the meter should swing 0 / 3.3 V at the
//       pin, and 0 / 5 V after the level shifter.
//   B — every pixel is set to dim white through the real driver: proves the
//       strip decodes what the driver sends.

namespace app::selftest {

void begin();
void tick();

}  // namespace app::selftest
