# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

WS2812B 16x16 LED matrix lamp firmware, PlatformIO + Arduino. One source tree builds for
two boards: ESP32-S3 (lamp A) and ESP8266 (lamp B). Two lamps talk over MQTT.

Branch `dev` is the v2 rebuild; `main` holds the previous, superseded implementation.

## Commands

```bash
pio run -e esp32s3          # lamp A (default env)
pio run -e d1_mini          # lamp B
pio test -e native          # core tests on the host — run these before touching core/
pio run -e esp32s3 -t upload
pio device monitor -b 115200   # 74880 on d1_mini
```

Host test dirs must be named `test_*` or PlatformIO ignores them.

## Architecture

Read `docs/architecture.md` first. The rules that matter when editing:

- **Dependency direction is one-way.** `core/` and `effects/` know nothing about the
  network, the database, the LED driver or Arduino — no `String`, no `millis()`, no
  `Serial`. That is what keeps effects testable on the host and identical on both boards.
  Board-specific code lives in `src/hal/<board>/` behind the interfaces in `src/hal/`.
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

`src/effects/Fire.cpp` is the reference effect and is documented line by line in
`docs/writing-effects.md`. Keep the two in sync when the effect API changes.

## Hardware constraints worth knowing

- ESP8266 drives the strip from I2S DMA on `GPIO3` (NeoPixelBus), not from FastLED's
  bit-bang — bit-banging blocks interrupts for ~7.7 ms per frame and tears async HTTP
  responses apart. See `docs/adr/0003-hardware-led-transport.md`.
- That also means the single I2S peripheral on ESP8266 is taken: no microphone there.
- The stock `esp32-s3-devkitc-1` board definition is the N8 variant with **no PSRAM**, so
  `platformio.ini` overrides `board_build.arduino.memory_type = qio_opi` along with the
  flash size. Miss `memory_type` and the build silently links the `qio_qspi` SDK, leaving
  PSRAM unavailable and `ESP.getPsramSize()` at 0 on a perfectly good N16R8. Verify with
  `pio run -e esp32s3 -t envdump | tr ',' '\n' | grep -oE '(qio|dio|opi)_(opi|qspi)'`.
- The `model` partition in `partitions/esp32s3.csv` is reserved for esp-sr wake word
  models. Do not repurpose it.

## Conventions

- **Code and comments in English. Documentation in `docs/` and `CONTEXT.md` in Russian.**
  User-facing strings (parameter labels, effect names) are Russian.
- Comments explain why, not what. The codebase is read by someone deciding whether a
  change is safe, not learning C++.
- `docs/adr/` records decisions that are hard to reverse. Add one only when the decision
  is costly to undo, surprising without context, and was a real trade-off.
