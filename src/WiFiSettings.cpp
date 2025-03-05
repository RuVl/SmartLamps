#include "WiFiSettings.h"

#include <MQTTClient.h>
#include <WiFiConnector.h>

WiFiSettingsClass& WiFiSettingsClass::instance() {
    static WiFiSettingsClass instance;
    return instance;
}

void WiFiSettingsClass::begin(SettingsAsyncWS& _settings, GyverDBFile& _db) {
    settings = &_settings;
    db = &_db;

    // Ставим режим, чтобы Settings знали в каком режиме работать
    WiFi.mode(WIFI_AP_STA);

    settings->begin();
    settings->onBuild(build);

    db->init(kk::wifi_ssid, "");
    db->init(kk::wifi_password, "");
    db->init(kk::wifi_connected, false);

    initConnector();
}

void WiFiSettingsClass::tick() {
    WiFiConnector.tick();
    settings->tick(); // implicit db tick
}

void WiFiSettingsClass::initConnector() {
    WiFiConnector.onConnect(onConnect);
    WiFiConnector.onError(onError);

    // ======= AP =======
    WiFiConnector.closeAP(WIFI_CLOSE_AP);
    WiFiConnector.setTimeout(WIFI_CONNECTION_TIMEOUT);

    WiFiConnector.setName(WIFI_AP_NAME);
#ifdef WIFI_AP_PASSWORD
    WiFiConnector.setPass(WIFI_AP_PASSWORD);
#endif

    LOG(sets::Logger::info() + "WiFi AP IP: ");
    LOG_LN(WiFi.softAPIP());

    // ======= STA =======
    // если логин задан - подключаемся
    const String ssid = db->get(kk::wifi_ssid);
    if (ssid.length()) {
        LOG_LN(sets::Logger::info() + "Connecting STA...");
        WiFiConnector.connect(ssid, db->get(kk::wifi_password));
    }
    
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void WiFiSettingsClass::onConnect() {
    instance().db->set(kk::wifi_connected, true);

    LOG(sets::Logger::info() + "Connected STA. IP: ");
    LOG_LN(WiFi.localIP());

    MQTTClient.reconnect();
    WiFiSettings.settings->updater().update(logger_name, logger);
}

void WiFiSettingsClass::onError() {
    instance().db->set(kk::wifi_connected, false);
    LOG_LN(sets::Logger::error() + "Can't connect to STA.");
    instance().settings->updater().update(logger_name, logger);
}

void WiFiSettingsClass::build(sets::Builder& b) {
    GyverDBFile *db = instance().db;
    {
        sets::Menu _(b, "WiFi");
        b.Input(kk::wifi_ssid, "SSID (only 2.4 GHz)");
        b.Pass(kk::wifi_password, "Password");
        b.LED(kk::wifi_connected, "Status");

        if (b.Button("Reconnect")) {
            LOG_LN(sets::Logger::info() + "Reconnecting STA...");
            db->update(); // сохраняем БД не дожидаясь таймаута
            WiFiConnector.connect(db->get(kk::wifi_ssid), db->get(kk::wifi_password));
        }

        b.Log(logger_name, logger);
    }

    MQTTClient.build(b);
}
