#ifndef WIFI_H
#define WIFI_H

#include <EEManager.h>
#include <GyverPortal.h>

#define WIFI_CONNECTION_TIMEOUT 15000
#define WIFI_AP_NAME "RuVlamp"

struct WiFiConfig {
    char ssid[32];
    char password[64];
};

void wifiInit(WiFiConfig&, EEManager&, GyverPortal&);

void wifiSetup();

#endif //WIFI_H
