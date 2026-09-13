// MQTT: plain topics in, JSON state out, Home Assistant discovery on connect.
// Protocol reference: docs/mqtt.md.

#include "Mqtt.h"

#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include "Config.h"
#include "Log.h"
#include "app/Lamp.h"
#include "core/Registry.h"
#include "hal/Database.h"

namespace net::mqtt {
namespace {

constexpr uint32_t kRetryMs = 5000;
constexpr uint16_t kKeepAliveS = 15;
constexpr size_t kMaxPayload = 256;

AsyncMqttClient client;

// AsyncMqttClient stores the pointers it is given, not copies. These live as
// long as the client does — handing it a temporary String's c_str() is how
// the previous firmware ended up with dangling host and credentials.
String g_host;
String g_user;
String g_pass;
String g_clientId;
String g_willTopic;
String g_cmdPrefix;
uint16_t g_port = 1883;

uint32_t g_lastAttempt = 0;
bool g_wantDiscovery = false;

// ---------------------------------------------------------------- outbound --

void publish(const String& topic, const char* payload, bool retain) {
    if (!client.connected()) return;
    client.publish(topic.c_str(), 1, retain, payload);
}

void publishDiscovery() {
    // Home Assistant "default" light schema: separate command topics, one
    // JSON state topic read through templates. Matches docs/mqtt.md exactly.
    JsonDocument doc;
    const String& name = lampName();
    doc["name"] = name;
    doc["uniq_id"] = name;
    doc["~"] = String(F("lamp/")) + name;
    doc["avty_t"] = "~/avail";
    doc["cmd_t"] = "~/cmd/on";
    doc["pl_on"] = "1";
    doc["pl_off"] = "0";
    doc["stat_t"] = "~/state";
    doc["stat_val_tpl"] = "{{ '1' if value_json.on else '0' }}";
    doc["bri_cmd_t"] = "~/cmd/brightness";
    doc["bri_scl"] = 100;
    doc["bri_stat_t"] = "~/state";
    doc["bri_val_tpl"] = "{{ value_json.brightness }}";
    doc["fx_cmd_t"] = "~/cmd/effect";
    doc["fx_stat_t"] = "~/state";
    doc["fx_val_tpl"] = "{{ value_json.effect }}";
    JsonArray list = doc["fx_list"].to<JsonArray>();
    for (core::EffectInfo* e = core::Registry::head(); e != nullptr; e = e->next)
        list.add(e->name);
    JsonObject dev = doc["dev"].to<JsonObject>();
    dev["ids"].to<JsonArray>().add(name);
    dev["name"] = String(F("SmartLamp ")) + name;
    dev["mdl"] = "SmartLamp v2";
    dev["mf"] = "RuVl";

    String out;
    serializeJson(doc, out);
    publish(String(F("homeassistant/light/")) + name + F("/config"), out.c_str(), true);
}

// ----------------------------------------------------------------- inbound --

void handleCommand(const char* cmd, const String& value) {
    app::Lamp& lamp = app::lamp();

    if (strcmp(cmd, "on") == 0) {
        lamp.setPower(value == "1" || value.equalsIgnoreCase("on") || value.equalsIgnoreCase("true"));
    } else if (strcmp(cmd, "brightness") == 0) {
        lamp.setBrightness(uint8_t(constrain(value.toInt(), 0, 100)));
    } else if (strcmp(cmd, "effect") == 0) {
        if (!lamp.selectEffect(value.c_str()))
            logWarn(String(F("MQTT: неизвестный эффект ")) + value);
    } else if (strncmp(cmd, "param/", 6) == 0) {
        if (!lamp.setParam(cmd + 6, int16_t(value.toInt())))
            logWarn(String(F("MQTT: неизвестный параметр ")) + (cmd + 6));
    } else {
        logWarn(String(F("MQTT: неизвестная команда ")) + cmd);
    }
}

void onMessage(char* topic, char* payload, AsyncMqttClientMessageProperties,
               size_t len, size_t index, size_t total) {
    // Commands are short; anything fragmented or oversized is not one.
    if (index != 0 || total != len || len > kMaxPayload) return;
    if (strncmp(topic, g_cmdPrefix.c_str(), g_cmdPrefix.length()) != 0) return;

    String value;
    value.reserve(len);
    for (size_t i = 0; i < len; ++i) value += payload[i];
    value.trim();

    handleCommand(topic + g_cmdPrefix.length(), value);
}

void onConnect(bool) {
    logInfo(F("MQTT: подключено"));
    client.subscribe((g_cmdPrefix + '#').c_str(), 1);
    publish(topic("avail"), "online", true);
    // Discovery and state go out from tick(), not from inside the callback.
    g_wantDiscovery = true;
}

void onDisconnect(AsyncMqttClientDisconnectReason reason) {
    logWarn(String(F("MQTT: отключено, причина ")) + int(reason));
}

}  // namespace

// ------------------------------------------------------------------ public --

void begin() {
    GyverDBFile& db = hal::database();
    db.init(kMqttHost, "");
    db.init(kMqttPort, 1883);
    db.init(kMqttUser, "");
    db.init(kMqttPass, "");

    client.onConnect(onConnect);
    client.onDisconnect(onDisconnect);
    client.onMessage(onMessage);
    client.setKeepAlive(kKeepAliveS);
}

bool configured() { return !hal::database().get(kMqttHost).toString().isEmpty(); }

void reconnect() {
    GyverDBFile& db = hal::database();
    g_host = db.get(kMqttHost).toString();
    g_host.trim();
    if (g_host.isEmpty()) return;
    if (WiFi.status() != WL_CONNECTED) return;

    g_port = uint16_t(db.get(kMqttPort).toInt());
    g_user = db.get(kMqttUser).toString();
    g_pass = db.get(kMqttPass).toString();
    g_clientId = lampName();
    g_willTopic = topic("avail");
    g_cmdPrefix = topic("cmd/");

    if (client.connected()) client.disconnect();

    client.setServer(g_host.c_str(), g_port);
    client.setClientId(g_clientId.c_str());
    if (!g_user.isEmpty()) client.setCredentials(g_user.c_str(), g_pass.c_str());
    client.setWill(g_willTopic.c_str(), 1, true, "offline");

    logInfo(String(F("MQTT: подключаюсь к ")) + g_host + ':' + g_port);
    g_lastAttempt = millis();
    client.connect();
}

void tick(uint32_t nowMs) {
    if (g_wantDiscovery && client.connected()) {
        g_wantDiscovery = false;
        publishDiscovery();
        publishState();
    }
    if (!client.connected() && configured() && WiFi.status() == WL_CONNECTED &&
        nowMs - g_lastAttempt >= kRetryMs) {
        reconnect();
    }
}

bool connected() { return client.connected(); }

void publishState() {
    if (!client.connected()) return;
    app::Lamp& lamp = app::lamp();

    JsonDocument doc;
    doc["on"] = lamp.isOn();
    doc["brightness"] = lamp.brightness();
    doc["effect"] = lamp.effectName();
    JsonObject params = doc["params"].to<JsonObject>();
    for (core::Param* p = lamp.params(); p != nullptr; p = p->next())
        params[p->key()] = p->get();
    doc["fps"] = lamp.fps();
    doc["rssi"] = WiFi.RSSI();

    String out;
    serializeJson(doc, out);
    publish(topic("state"), out.c_str(), true);
}

}  // namespace net::mqtt
