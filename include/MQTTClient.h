#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include "config.h"

#include <AsyncMqttClient.h>
#include <GyverDBFile.h>
#include <SettingsAsyncWS.h>

class GyverDBFile;

class MQTTClientClass {
public:
    static MQTTClientClass& instance();
    MQTTClientClass(MQTTClientClass const&) = delete;
    MQTTClientClass& operator=(MQTTClientClass const&) = delete;

    void begin(AsyncMqttClient&, GyverDBFile& _db);
    void reconnect();

    void buildUI(sets::Builder&);

private:
    MQTTClientClass() {}

    size_t logger_name = "mqtt_logger"_h;
    sets::Logger logger{LOG_BUFFER_CAPACITY};

    AsyncMqttClient* client = nullptr;
    GyverDBFile* db = nullptr;

    void onConnect(bool sessionPresent);
    void onDisconnect(AsyncMqttClientDisconnectReason);
    void onMessage(const char* topic, const char* payload, AsyncMqttClientMessageProperties, size_t len, size_t index, size_t total);

protected:
    DB_KEYS(
        kk,
        mqtt_ip,
        mqtt_port,
        mqtt_username,
        mqtt_password,
        mqtt_topic,
        mqtt_connected
    )
};

#define MQTTClient MQTTClientClass::instance()

#endif //MQTTCLIENT_H
