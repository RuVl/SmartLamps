#include "MQTTClient.h"

#include <ESP8266WiFi.h>
#include <WiFiSettings.h>

using namespace std::placeholders;

MQTTClientClass& MQTTClientClass::instance() {
    static MQTTClientClass instance;
    return instance;
}

void MQTTClientClass::begin(AsyncMqttClient& _client, GyverDBFile& _db) {
    client = &_client;
    db = &_db;

    _db.init(mqtt_ip, "");
    _db.init(mqtt_port, 0);
    _db.init(mqtt_username, "");
    _db.init(mqtt_password, "");
    _db.init(mqtt_topic, "");
    _db.init(mqtt_connected, false);

    client->onConnect(std::bind(&MQTTClientClass::onConnect, this, _1));
    client->onDisconnect(std::bind(&MQTTClientClass::onDisconnect, this, _1));
    client->onMessage(std::bind(&MQTTClientClass::onMessage, this, _1, _2, _3, _4, _5, _6));

    reconnect();
}

void MQTTClientClass::reconnect() {
    const String ip = db->get(mqtt_ip);
    if (!ip.length()) return; // IP server unset

    if (WiFi.status() != WL_CONNECTED) {
        LOG_LN(sets::Logger::warn() + "WiFi not connected!");
        return;
    }

    LOG_LN(sets::Logger::info() + "Reconnecting MQTT...");

    // Disconnect from server, if connected
    if (client->connected()) client->disconnect();

    // Set new server ip and port
    client->setServer(ip.c_str(), db->get(mqtt_port));
    client->setCredentials(db->get(mqtt_username).c_str(), db->get(mqtt_password).c_str());

    client->connect();
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void MQTTClientClass::onMessage(const char* topic, const char* payload, AsyncMqttClientMessageProperties properties,
                                const size_t len, size_t index, size_t total) {
    LOG(sets::Logger::info() + "Message received on topic: ");
    LOG_LN(topic);

    LOG(sets::Logger::info() + "Message: ");
    for (unsigned int i = 0; i < len; i++) {
        LOG(payload[i]);
    }
    LOG_LN();
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void MQTTClientClass::onConnect(bool sessionPresent) {
    db->set(mqtt_connected, true);
    LOG_LN(sets::Logger::info() + "Connected to MQTT");

    const String topic = db->get(mqtt_topic);
    const uint16_t packetIdSub = client->subscribe(topic.c_str(), 1);
    if (packetIdSub == 0) {
        LOG_LN(sets::Logger::error() + "Cannot subscribe to topic");
        return;
    }

    client->publish(topic.c_str(), 1, false, "Connected");
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void MQTTClientClass::onDisconnect(const AsyncMqttClientDisconnectReason reason) {
    db->set(mqtt_connected, false);
    LOG(sets::Logger::info() + "Disconnected from MQTT: ");
    LOG_LN((int8_t)reason);
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void MQTTClientClass::buildUI(sets::Builder& b) {
    {
        sets::Menu _(b, "MQTT");
        b.Input(mqtt_ip, "Server IP");
        b.Number(mqtt_port, "Server port");
        b.Input(mqtt_username, "Username");
        b.Pass(mqtt_password, "Password");
        b.Input(mqtt_topic, "Topic");
        b.LED(mqtt_connected, "Status");

        if (b.Button("Reconnect")) {
            db->update(); // сохраняем БД не дожидаясь таймаута
            reconnect();
        }
        b.Log(logger_name, logger);
    }
}
