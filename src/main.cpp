#include <Arduino.h>
#include <EEManager.h>
#include <GyverPortal.h>
// #include <PubSubClient.h>

#include "wifi.h"

struct LampData {
    WiFiConfig wifi_config = wifi_config;
} data;

EEManager memory(data);
GyverPortal portal;

// WiFiClient espClient;
// PubSubClient client(espClient);

void setup() {
    Serial.begin(9600); // запускаем сериал для отладки

    // Initialize memory
    EEPROM.begin(memory.blockSize());
    memory.begin(0, 0);

    wifiInit(data.wifi_config, memory, portal);
    wifiSetup();


    // client.setServer(mqtt_server, mqtt_port);
    // client.setCallback(callback);
}

void loop() {
    memory.tick(); // проверяем обновление настроек
    portal.tick(); // пинаем портал
}