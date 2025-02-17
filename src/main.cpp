#include <Arduino.h>
#include <EEManager.h>
#include <GyverPortal.h>
#include <PubSubClient.h>

#include "wifi.h"
#include "mqtt.h"

struct LampData {
    WiFiConfig wifi_config = wifi_config;
} data;

EEManager memory(data);
GyverPortal portal;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    Serial.begin(9600); // запускаем сериал для отладки

    // Initialize memory
    EEPROM.begin(memory.blockSize());
    memory.begin(0, 0);

    // Initialize Wi-Fi
    wifiInit(data.wifi_config, memory, portal);
    wifiSetup();

    // Initialize mqtt
    setupMQTT(client);
}

void loop() {
    memory.tick(); // проверяем обновление настроек
    portal.tick(); // пинаем портал
    mqttTick();    // опрашиваем mqtt
}