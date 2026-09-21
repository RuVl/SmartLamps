# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

WS2812B 16x16 LED matrix lamp firmware for a WeMos D1 mini (ESP8266), PlatformIO +
Arduino. The lamp can be paired with another lamp over MQTT; the partner's hardware and
firmware are not this repository's concern.

Branch `dev` is the v2 rebuild; `main` holds the previous, superseded implementation.

## Commands

```bash
pio run                     # firmware (env d1_mini)
pio run -e d1_mini_mem      # the same with heap telemetry, see docs/memory.md
pio test -e native          # core tests on the host - run these before touching core/
pio run -e sim && .pio/build/sim/program   # effect preview at http://localhost:8266
pio run -t upload
pio device monitor -b 74880
```

Host test dirs must be named `test_*` or PlatformIO ignores them.

**The user flashes boards themselves.** Never run `pio run -t upload` or esptool writes;
build, verify, and say the build is ready. Reading the serial port for logs is fine.

## Architecture

Read `docs/architecture.md` first. The rules that matter when editing:

- **Dependency direction is one-way.** `core/` and `effects/` know nothing about the
  network, the database, the LED driver or Arduino - no `String`, no `millis()`, no
  `Serial`. That is what keeps effects testable on the host and identical in the
  simulator. Board code lives in `src/hal/*.cpp` behind the interfaces in `src/hal/*.h`;
  the simulator's implementations of the same interfaces are in `src/sim/hal/`.
- **Effects are self-registering.** `REGISTER_EFFECT` at the bottom of an effect's .cpp
  is the only place it is mentioned. There is no enum, no name list, no switch. Adding an
  effect touches exactly one file in `src/effects/`.
- **Params are declared once.** A `core::Param` member produces the web panel widget, the
  database key (`hash("<effect>.<param>")`), the MQTT topic and the JSON state field.
  Never write a key or a default twice.
- **The active effect lives in a static arena**, built with placement new. Never `new` an
  effect. `REGISTER_EFFECT` static_asserts the size against `EFFECT_ARENA_SIZE`.
- **Never index the pixel buffer by coordinates.** Go through `Frame::at(x, y)` / `xy()`,
  which apply the matrix geometry from `core/Matrix.h`.
- **Effects advance by `dtMs`, not by call count.** The lamp renders at a fixed rate; an
  effect that counts frames slows down under load.
- **Brightness, gamma and the current limit belong to PostFX**, not to effects.
- **Effect state is fixed-size.** Particles, trails, heat maps are plain arrays sized at
  compile time; the whole effect must fit the arena, and it must not allocate in `render`.
- **Nothing heavy inside a panel callback.** `sets::Builder` callbacks run in the
  SDK sys context (5 KB of stack, and only because of `disable_extra4k_at_link_time`): no flash
  writes, no `WiFi.mode`, no restart - record a `Pending` request and act from `tick()`.
  Unsolicited WebSocket pushes from `loop()` go through the throttle in `WebUi.cpp`.

`src/effects/Fire.cpp` is the reference effect and is documented line by line in
`docs/writing-effects.md`. Keep the two in sync when the effect API changes.

## Hardware constraints worth knowing

- The strip is driven from I2S DMA on `GPIO3` (NeoPixelBus), not from FastLED's
  bit-bang - bit-banging blocks interrupts for ~7.7 ms per frame and tears async HTTP
  responses apart. See `docs/adr/0003-hardware-led-transport.md`. FastLED stays for
  colour math only.
- That also means the single I2S peripheral is taken: no microphone, no sound-reactive
  effects.
- ~40 KB of free heap, shared with the network stack. `tools/memory_budget.py` runs
  after every build and fails it without heap margin; see `docs/memory.md`.

## Conventions

- **Code and comments in English. Documentation in `docs/` and `CONTEXT.md` in Russian.**
  User-facing strings (parameter labels, effect names) are Russian.
- Comments explain why, not what. The codebase is read by someone deciding whether a
  change is safe, not learning C++.
- `docs/adr/` records decisions that are hard to reverse. Add one only when the decision
  is costly to undo, surprising without context, and was a real trade-off.
