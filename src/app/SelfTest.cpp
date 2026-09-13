#include "SelfTest.h"

#include <Arduino.h>
#include <FastLED.h>

#include "hal/LedDriver.h"

namespace app::selftest {
namespace {

constexpr uint16_t kPixelCount = MATRIX_WIDTH * MATRIX_HEIGHT;
constexpr uint32_t kPhaseMs = 10000;
constexpr uint8_t kWhite = 24;  // dim: under 1 A for 256 LEDs, safe on USB power

CRGB pixels[kPixelCount];
bool driverStarted = false;
uint32_t phaseStart = 0;
bool phaseToggle = true;
bool level = false;
uint32_t lastToggle = 0;

void enterToggle() {
    // Take the pin back from the DMA driver. NeoPixelBus does not have an
    // end(), so the I2S peripheral keeps running; plain GPIO writes still win
    // on the ESP8266 once the pin function is switched back.
    pinMode(LED_DATA_PIN, OUTPUT);
    Serial.println(F("SELFTEST A: GPIO toggles 1 Hz — meter 0 / 3.3 V on the pin"));
}

void enterWhite() {
    if (!driverStarted) {
        hal::ledDriver().begin(pixels, kPixelCount);
        driverStarted = true;
    }
    fill_solid(pixels, kPixelCount, CRGB(kWhite, kWhite, kWhite));
    Serial.println(F("SELFTEST B: all pixels dim white via driver"));
}

}  // namespace

void begin() {
    Serial.println(F("SELFTEST build"));
    phaseStart = millis();
    enterToggle();
}

void tick() {
    const uint32_t now = millis();

    if (now - phaseStart >= kPhaseMs) {
        phaseStart = now;
        phaseToggle = !phaseToggle;
        if (phaseToggle) enterToggle();
        else enterWhite();
    }

    if (phaseToggle) {
        if (now - lastToggle >= 500) {
            lastToggle = now;
            level = !level;
            digitalWrite(LED_DATA_PIN, level ? HIGH : LOW);
        }
    } else {
        // Re-send every 100 ms so a strip that just got power catches up.
        static uint32_t lastShow = 0;
        if (now - lastShow >= 100) {
            lastShow = now;
            hal::ledDriver().show(255);
        }
    }
}

}  // namespace app::selftest
