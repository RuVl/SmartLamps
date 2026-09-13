#include "Config.h"

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include "hal/Database.h"

namespace net {
namespace {

String g_lampName;
String g_pairName;

String defaultName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[16];
    snprintf(buf, sizeof(buf), "lamp-%02x%02x", mac[4], mac[5]);
    return String(buf);
}

}  // namespace

void refreshNames() {
    GyverDBFile& db = hal::database();
    db.init(kLampName, "");
    db.init(kPairName, "");
    String name = db.get(kLampName).toString();
    name.trim();
    g_lampName = name.length() ? name : defaultName();
    g_pairName = db.get(kPairName).toString();
    g_pairName.trim();
}

const String& lampName() {
    if (g_lampName.isEmpty()) refreshNames();
    return g_lampName;
}

const String& pairName() {
    if (g_lampName.isEmpty()) refreshNames();
    return g_pairName;
}

String topicOf(const String& name, const char* suffix) {
    String t;
    t.reserve(6 + name.length() + strlen(suffix));
    t += F("lamp/");
    t += name;
    t += '/';
    t += suffix;
    return t;
}

}  // namespace net
