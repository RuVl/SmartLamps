#include "MQTTClient.h"

#include <ESP8266WiFi.h>
#include <WiFiSettings.h>

MQTTClientClass& MQTTClientClass::instance() {
    static MQTTClientClass instance;
    return instance;
}

void MQTTClientClass::begin(AsyncMqttClient& _client, GyverDBFile& _db) {
    client = &_client;
    db = &_db;

    db->init(kk::mqtt_ip, "");
    db->init(kk::mqtt_port, 0);
    db->init(kk::mqtt_user, "");
    db->init(kk::mqtt_password, "");
    db->init(kk::mqtt_topic, "");
    db->init(kk::mqtt_connected, false);

    client->onConnect(onConnect);
    client->onDisconnect(onDisconnect);
    client->onMessage(onMessage);

    reconnect();
}

void MQTTClientClass::reconnect() {
    const String ip = db->get(kk::mqtt_ip);
    if (!ip.length()) return; // IP server unset

    if (WiFi.status() != WL_CONNECTED) {
        LOG_LN(sets::Logger::warn() + "WiFi not connected!");
        return;
    }

    LOG_LN(sets::Logger::info() + "Reconnecting MQTT...");

    // Disconnect from server, if connected
    if (client->connected()) client->disconnect();

    // Set new server ip and port
    client->setServer(ip.c_str(), db->get(kk::mqtt_port));
    client->setCredentials(db->get(kk::mqtt_user).c_str(), db->get(kk::mqtt_password).c_str());

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
    GyverDBFile* db = instance().db;
    AsyncMqttClient* client = instance().client;
    
    db->set(kk::mqtt_connected, true);
    LOG_LN(sets::Logger::info() + "Connected to MQTT");

    const String topic = db->get(kk::mqtt_topic);
    const uint16_t packetIdSub = client->subscribe(topic.c_str(), 1);
    if (packetIdSub == 0) {
        LOG_LN(sets::Logger::error() + "Cannot subscribe to topic");
        return;
    }

    client->publish(topic.c_str(), 1, false, "Connected");
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void MQTTClientClass::onDisconnect(const AsyncMqttClientDisconnectReason reason) {
    instance().db->set(kk::mqtt_connected, false);
    LOG(sets::Logger::info() + "Disconnected from MQTT: ");
    LOG_LN((int8_t)reason);
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void MQTTClientClass::build(sets::Builder& b) {
    {
        sets::Menu _(b, "MQTT");
        b.Input(kk::mqtt_ip, "Server IP");
        b.Number(kk::mqtt_port, "Server port");
        b.Input(kk::mqtt_user, "User");
        b.Pass(kk::mqtt_password, "Password");
        b.Input(kk::mqtt_topic, "Topic");
        b.LED(kk::mqtt_connected, "Status");

        if (b.Button("Reconnect")) {
            instance().db->update(); // сохраняем БД не дожидаясь таймаута
            instance().reconnect();
        }
        b.Log(logger_name, logger);
    }
}
