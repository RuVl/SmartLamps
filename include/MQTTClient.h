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

    static void build(sets::Builder&);

private:
    MQTTClientClass() {}

    static inline size_t logger_name = "mqtt_logger"_h;
    static inline sets::Logger logger{LOG_BUFFER_CAPACITY};

    AsyncMqttClient* client = nullptr;
    GyverDBFile* db = nullptr;

    static void onConnect(bool sessionPresent);
    static void onDisconnect(AsyncMqttClientDisconnectReason);
    static void onMessage(const char* topic, const char* payload, AsyncMqttClientMessageProperties, size_t len, size_t index, size_t total);
};

#define MQTTClient MQTTClientClass::instance()

#endif //MQTTCLIENT_H
