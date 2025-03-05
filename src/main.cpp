#include "config.h"

#include <Arduino.h>
#include <EncButton.h>

#include "WiFiSettings.h"
#include "MQTTClient.h"
#include "EffectController.h"

GyverDBFile db(&LittleFS, DB_PATH); // NOLINT(*-interfaces-global-init)
SettingsAsyncWS settings("Settings", &db);

AsyncMqttClient client;
ButtonT<D3> btn(INPUT_PULLUP, HIGH);

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // базу данных запускаем до подключения к точке
#ifdef ESP32
    LittleFS.begin(true); // format on fail
#else
    LittleFS.begin();
#endif
    db.begin();

    WiFiSettings.begin(settings, db);
    MQTTClient.begin(client, db);

    // btn.init();
}

void loop() {
    WiFiSettings.tick();
    // EffectController.tick();
    // btn.tick();
 }