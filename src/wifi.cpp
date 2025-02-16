#include "wifi.h"

#include "Timer.h"

#include <ESP8266WiFi.h>
#include <ESP8266WiFiType.h>
#include <HardwareSerial.h>

// Initialize with wifiInit()
WiFiConfig* __wifi_config; // NOLINT(*-reserved-identifier)
EEManager* __memory; // NOLINT(*-reserved-identifier)
GyverPortal* __portal; // NOLINT(*-reserved-identifier)



void uiBuild() {
    GP.BUILD_BEGIN();
    GP.THEME(GP_DARK);

    GP.TITLE("WiFi Setup");
    GP.BREAK();

    GP.FORM_BEGIN("/wifi_config");

    GP.LABEL("Name:");
    GP.TEXT("ssid");
    GP.BREAK();

    GP.LABEL("Password:");
    GP.TEXT("password");
    GP.BREAK();

    GP.FORM_SEND("Connect");

    GP.FORM_END();
    GP.BUILD_END();
}

void action(GyverPortal& portal) {
    if (portal.form("/wifi_config")) {
        Serial.println("Form update captured");

        portal.copyStr("ssid", __wifi_config->ssid, 32);
        portal.copyStr("password", __wifi_config->password, 64);

        __memory->updateNow();
        ESP.restart(); // NOLINT(*-static-accessed-through-instance)
    }
}

void localPortal() {
    __portal->start();
    Serial.println(WiFi.softAPIP());

    while (__portal->tick());
}

void wifiInit(WiFiConfig& _wifi_config, EEManager& _memory, GyverPortal& _portal) {
    __wifi_config = &_wifi_config;
    __memory = &_memory;
    __portal = &_portal;

    __portal->attachBuild(uiBuild);
    __portal->attach(action);
}

void wifiSetup() {
    Serial.println("Connecting wifi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(__wifi_config->ssid, __wifi_config->password);

    Timer timer(WIFI_CONNECTION_TIMEOUT);

    pinMode(LED_BUILTIN, OUTPUT); // led indicator
    digitalWrite(LED_BUILTIN, HIGH);

    while (WiFi.status() != WL_CONNECTED) {
        if (timer.period()) {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("Failed to connect wifi.");

            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_AP_NAME);

            localPortal();
            return;
        }
        yield(); // Даём ESP8266 обработать фоновые задачи
    }

    Serial.println("WiFi connected!");
}
