// SmartLamp v2.
//
// main() stays empty on purpose: everything is wired up in app/, so the entry
// point never becomes the place where unrelated subsystems meet.
//
// Current state: the core (Matrix, Frame, Param, Effect, Registry) and the
// HAL interfaces are in place, the implementations are not. See
// docs/architecture.md for the plan and the order the layers land in.

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println(F("SmartLamp v2 — core scaffolding"));
}

void loop() {
}
