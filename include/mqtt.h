#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

#define MQTT_SERVER_IP "103.137.251.15"
#define MQTT_SERVER_PORT 1883

#define MQTT_USER "RuVlamp"
#define MQTT_PASSWORD "LoveForever"

#define MQTT_TOPIC "test"

void setupMQTT(PubSubClient&);

void connectMQTT();

void mqttTick();

#endif //MQTT_H
