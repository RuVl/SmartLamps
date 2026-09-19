// MQTT: plain topics in, JSON state out, Home Assistant discovery on connect.
// Protocol reference: docs/mqtt.md.

#include "Mqtt.h"

#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <atomic>
#include <string.h>
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
#include "hal/Mailbox.h"

namespace net::mqtt
{
    namespace
    {
        constexpr uint32_t kRetryMs = 5000;
        constexpr uint16_t kKeepAliveS = 15;

        AsyncMqttClient client;

        // AsyncMqttClient stores the pointers it is given, not copies. These live as
        // long as the client does - handing it a temporary String's c_str() is how
        // the previous firmware ended up with dangling host and credentials.
        String g_host;
        String g_user;
        String g_pass;
        String g_clientId;
        String g_willTopic;
        String g_cmdPrefix;
        uint16_t g_port = 1883;

        uint32_t g_lastAttempt = 0;
        // "Reconnect" on a live connection: disconnect first, connect again as
        // soon as the TCP side reports the close - not after the retry period.
        bool g_reconnectRequested = false;

        // AsyncMqttClient fires its callbacks from the TCP stack. They only post
        // what happened here; tick() does the logging, the subscribe and the
        // publishes from loop(), where a String and a full stack are safe.
        hal::Mailbox<int8_t> g_reason;
        hal::Mailbox<bool> g_connected;
        String g_lastError;

        // Inbound commands wait here for tick(). Fixed slots, not Strings: the
        // callback must not allocate, and a command is a topic suffix plus a
        // short value.
        // Single producer (the callback), single consumer (tick()), so the two
        // indices need no lock: each is written by one side only.
        struct Command
        {
            char topic[32];
            char value[64];
        };
        constexpr uint8_t kQueueSize = 4; // Home Assistant sends on + brightness + effect in one burst
        Command g_queue[kQueueSize];
        std::atomic<uint8_t> g_queueHead{0};
        std::atomic<uint8_t> g_queueTail{0};

        const __FlashStringHelper* reasonText(AsyncMqttClientDisconnectReason r)
        {
            switch (r)
            {
            case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
                return F("соединение разорвано - хост недоступен, порт закрыт или брокер молчит");
            case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
                return F("брокер не принимает версию протокола");
            case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
                return F("брокер отверг идентификатор клиента (имя лампы)");
            case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
                return F("брокер недоступен");
            case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
                return F("неверный формат логина или пароля");
            case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
                return F("брокер отверг логин или пароль");
            case AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE:
                return F("не хватило памяти под пакет");
            case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
                return F("отпечаток TLS не совпал");
            }
            return F("неизвестная причина");
        }

        // ---------------------------------------------------------------- outbound --

        void publish(const String& topic, const char* payload, bool retain)
        {
            if (!client.connected()) return;
            client.publish(topic.c_str(), 1, retain, payload);
        }

        void publishDiscovery()
        {
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

        void handleCommand(const char* cmd, const String& value)
        {
            app::Lamp& lamp = app::lamp();

            if (strcmp(cmd, "on") == 0)
            {
                lamp.setPower(value == "1" || value.equalsIgnoreCase("on") || value.equalsIgnoreCase("true"));
            }
            else if (strcmp(cmd, "brightness") == 0)
            {
                lamp.setBrightness(uint8_t(constrain(value.toInt(), 0, 100)));
            }
            else if (strcmp(cmd, "effect") == 0)
            {
                if (!lamp.selectEffect(value.c_str()))
                    logWarn(String(F("MQTT: неизвестный эффект ")) + value);
            }
            else if (strncmp(cmd, "param/", 6) == 0)
            {
                if (!lamp.setParam(cmd + 6, int16_t(value.toInt())))
                    logWarn(String(F("MQTT: неизвестный параметр ")) + (cmd + 6));
            }
            else
            {
                logWarn(String(F("MQTT: неизвестная команда ")) + cmd);
            }
        }

        void onMessage(char* topic, char* payload, AsyncMqttClientMessageProperties,
                       size_t len, size_t index, size_t total)
        {
            // Commands are short; anything fragmented or oversized is not one.
            if (index != 0 || total != len || len >= sizeof(Command::value)) return;
            if (strncmp(topic, g_cmdPrefix.c_str(), g_cmdPrefix.length()) != 0) return;
            const char* suffix = topic + g_cmdPrefix.length();
            if (strlen(suffix) >= sizeof(Command::topic)) return;

            const uint8_t head = g_queueHead.load(std::memory_order_relaxed);
            const uint8_t next = uint8_t((head + 1) % kQueueSize);
            if (next == g_queueTail.load(std::memory_order_acquire)) return; // full: loop() is behind, drop

            Command& c = g_queue[head];
            strcpy(c.topic, suffix);
            memcpy(c.value, payload, len);
            c.value[len] = '\0';
            g_queueHead.store(next, std::memory_order_release);
        }

        void drainCommands()
        {
            uint8_t tail = g_queueTail.load(std::memory_order_relaxed);
            while (tail != g_queueHead.load(std::memory_order_acquire))
            {
                Command& c = g_queue[tail];
                String value(c.value);
                value.trim();
                handleCommand(c.topic, value);
                tail = uint8_t((tail + 1) % kQueueSize);
                g_queueTail.store(tail, std::memory_order_release);
            }
        }

        void onConnect(bool) { g_connected.set(true); }

        void onDisconnect(AsyncMqttClientDisconnectReason reason) { g_reason.set(int8_t(reason)); }
    }

    // ------------------------------------------------------------------ public --

    void begin()
    {
        GyverDBFile& db = hal::database();
        db.init(kMqttHost, "");
        db.init(kMqttPort, 1883);
        db.init(kMqttUser, "");
        db.init(kMqttPass, "");

        g_host = db.get(kMqttHost).toString();
        g_host.trim();

        client.onConnect(onConnect);
        client.onDisconnect(onDisconnect);
        client.onMessage(onMessage);
        client.setKeepAlive(kKeepAliveS);
    }

    // From the cached host, not the database: this is asked every loop for
    // the status pixel, and a String out of GyverDB is a heap allocation.
    bool configured() { return !g_host.isEmpty(); }

    void reconnect()
    {
        GyverDBFile& db = hal::database();
        g_host = db.get(kMqttHost).toString();
        g_host.trim();
        if (g_host.isEmpty())
        {
            g_lastError = F("сервер не задан");
            logInfo(F("MQTT: сервер не задан"));
            return;
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            g_lastError = F("нет WiFi");
            logWarn(F("MQTT: нет WiFi, подключусь после сети"));
            return;
        }

        g_port = uint16_t(db.get(kMqttPort).toInt());
        g_user = db.get(kMqttUser).toString();
        g_pass = db.get(kMqttPass).toString();
        g_clientId = lampName();
        g_willTopic = topic("avail");
        g_cmdPrefix = topic("cmd/");

        if (client.connected())
        {
            // connect() on a connected client is a no-op in AsyncMqttClient, and
            // the TCP close is asynchronous: ask for it and finish from tick().
            g_reconnectRequested = true;
            logInfo(F("MQTT: переподключаюсь"));
            client.disconnect();
            return;
        }

        client.setServer(g_host.c_str(), g_port);
        client.setClientId(g_clientId.c_str());
        if (!g_user.isEmpty()) client.setCredentials(g_user.c_str(), g_pass.c_str());
        client.setWill(g_willTopic.c_str(), 1, true, "offline");

        logInfo(String(F("MQTT: подключаюсь к ")) + g_host + ':' + g_port);
        g_lastError = "";
        g_lastAttempt = millis();
        client.connect();
    }

    void tick(uint32_t nowMs)
    {
        int8_t code = 0;
        if (g_reason.take(code))
        {
            const auto reason = AsyncMqttClientDisconnectReason(code);
            g_lastError = String(reasonText(reason)) + F(" [") + int(reason) + ']';
            logWarn(String(F("MQTT: отключено: ")) + g_lastError);
            if (g_reconnectRequested)
            {
                g_reconnectRequested = false;
                g_lastAttempt = 0; // reconnect on this very tick
            }
        }

        bool connectedNow = false;
        if (g_connected.take(connectedNow) && client.connected())
        {
            logInfo(F("MQTT: подключено"));
            client.subscribe((g_cmdPrefix + '#').c_str(), 1);
            publish(topic("avail"), "online", true);
            publishDiscovery();
            publishState();
        }

        drainCommands();
        if (!client.connected() && configured() && WiFi.status() == WL_CONNECTED &&
            nowMs - g_lastAttempt >= kRetryMs)
        {
            reconnect();
        }
    }

    bool connected() { return client.connected(); }

    String status()
    {
        if (client.connected()) return String(F("подключено к ")) + g_host + ':' + g_port;
        if (!configured()) return F("сервер не задан");
        String s = F("не подключено");
        if (!g_lastError.isEmpty()) s += String(F(" · ")) + g_lastError;
        return s;
    }

    void publishState()
    {
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
}
