#include "Net.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"
#include "Mqtt.h"
#include "Ota.h"
#include "WebUi.h"
#include "Wifi.h"
#include "app/Lamp.h"

namespace net {
namespace {

constexpr size_t kLogBytes = 1024;
sets::Logger g_log(kLogBytes);
bool g_logDirty = false;

void append(const String& prefix, const String& s) {
    g_log.print(prefix);
    g_log.println(s);
    Serial.println(s);
    g_logDirty = true;
}

// One place decides what the corner pixel shows.
void updateStatus() {
    app::Lamp& lamp = app::lamp();
    if (ota::active()) return;  // OTA sets its own status and owns it until reboot

    app::Status s = app::Status::Ok;
    if (!wifi::connected()) {
        s = wifi::accessPointUp() ? app::Status::AccessPoint : app::Status::NoWifi;
    } else if (mqtt::configured() && !mqtt::connected()) {
        s = app::Status::NoBroker;
    }
    lamp.setStatus(s);
}

}  // namespace

sets::Logger& log() { return g_log; }
void logInfo(const String& s) { append(sets::Logger::info(), s); }
void logWarn(const String& s) { append(sets::Logger::warn(), s); }
void logError(const String& s) { append(sets::Logger::error(), s); }

void begin() {
    refreshNames();
    logInfo(String(F("Лампа ")) + lampName());

    // Order matters: WiFi sets the radio mode the panel's server needs,
    // and OTA registers its callbacks before WiFi can call start() on it.
    ota::begin();
    wifi::begin();
    web::begin();
    mqtt::begin();
}

void tick(uint32_t nowMs) {
    wifi::tick();
    web::tick();
    mqtt::tick(nowMs);
    ota::tick();

    updateStatus();

    if (app::lamp().consumeChanged()) {
        mqtt::publishState();
        web::pushState();
    }
    if (g_logDirty) {
        g_logDirty = false;
        web::pushLog();
    }
}

}  // namespace net
