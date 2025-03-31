#ifndef WiFiSETTINGS_H
#define WiFiSETTINGS_H

#include "config.h"

#include <GyverDBFile.h>
#include <MQTTClient.h>
#include <SettingsAsyncWS.h>

class WiFiSettingsClass {
public:
    static WiFiSettingsClass& instance();
    WiFiSettingsClass(WiFiSettingsClass const&) = delete;
    void operator=(WiFiSettingsClass const&) = delete;

    void begin(SettingsAsyncWS&, GyverDBFile&);
    void tick() const;

private:
    WiFiSettingsClass() {}

    size_t logger_name = "wifi_logger"_h;
    sets::Logger logger{LOG_BUFFER_CAPACITY};

    SettingsAsyncWS* settings = nullptr;
    GyverDBFile* db = nullptr;

    void initConnector();

    void onConnect();
    void onError();

    void buildUI(sets::Builder&);

protected:
    DB_KEYS(
        kk,
        wifi_ssid,
        wifi_password,
        wifi_connected
    )

    friend MQTTClientClass;
};

#define WiFiSettings WiFiSettingsClass::instance()

#endif //WiFiSETTINGS_H
