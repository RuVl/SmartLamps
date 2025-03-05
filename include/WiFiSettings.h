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
    void tick();

private:
    WiFiSettingsClass() {}

    static inline size_t logger_name = "wifi_logger"_h;
    static inline sets::Logger logger{LOG_BUFFER_CAPACITY};

    SettingsAsyncWS* settings = nullptr;
    GyverDBFile* db = nullptr;

    void initConnector();

    static void onConnect();
    static void onError();

    static void build(sets::Builder&);

    friend MQTTClientClass;
};

#define WiFiSettings WiFiSettingsClass::instance()

#endif //WiFiSETTINGS_H
