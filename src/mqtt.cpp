#include "mqtt.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <HardwareSerial.h>

PubSubClient* __client; // NOLINT(*-reserved-identifier)

// ---- Подключение к MQTT ----
void connectMQTT() {
    while (!__client->connected()) {
        Serial.print("Attempting MQTT connection...");

        if (__client->connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("connected!");
            __client->publish(MQTT_TOPIC, "Connected");
            __client->subscribe(MQTT_TOPIC);
        } else {
            Serial.print("failed, rc=");
            Serial.print(__client->state());
            Serial.println(" retrying in 5 seconds...");
            delay(5000);
        }
    }
}

// опрашиваем mqtt
void mqttTick() {
    if (WiFi.status() != WL_CONNECTED) return;  // wifi не подключен
    if (!__client->connected()) connectMQTT();
    else {
        // nice
    }
    __client->loop();
}

void callback(const char* topic, const uint8_t* payload, const unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);

    Serial.print("Message: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print(payload[i]);
    }
    Serial.println();

    // ---- Здесь можно мигать светодиодом ----
    digitalWrite(LED_BUILTIN, LOW);  // Включить
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH); // Выключить
}

void setupMQTT(PubSubClient& client) {
    __client = &client;

    client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
    client.setCallback(callback);
}
