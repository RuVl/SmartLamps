#include "WiFiSettings.h"

#include <WiFiConnector.h>
#include <MQTTClient.h>
#include <EffectController.h>

using namespace std::placeholders;

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
    settings->onBuild(std::bind(&WiFiSettingsClass::buildUI, this, _1));

    _db.init(wifi_ssid, "");
    _db.init(wifi_password, "");
    _db.init(wifi_connected, false);

    initConnector();
}

void WiFiSettingsClass::tick() const {
    WiFiConnector.tick();
    settings->tick(); // implicit db tick
}

void WiFiSettingsClass::initConnector() {
    WiFiConnector.onConnect(std::bind(&WiFiSettingsClass::onConnect, this));
    WiFiConnector.onError(std::bind(&WiFiSettingsClass::onError, this));

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
    LOG_LN(sets::Logger::info() + "Connecting STA...");
    WiFiConnector.connect(db->get(wifi_ssid), db->get(wifi_password));

    settings->updater().update(logger_name, logger);
}

void WiFiSettingsClass::onConnect() {
    db->set(wifi_connected, true);

    LOG(sets::Logger::info() + "Connected STA. IP: ");
    LOG_LN(WiFi.localIP());

    MQTTClient.reconnect();
    settings->updater().update(logger_name, logger);
}

void WiFiSettingsClass::onError() {
    db->set(wifi_connected, false);
    LOG_LN(sets::Logger::error() + "Can't connect to STA.");
    settings->updater().update(logger_name, logger);
}

void WiFiSettingsClass::buildUI(sets::Builder& b) {
    {
        sets::Menu _(b, "WiFi");
        b.Input(wifi_ssid, "SSID (only 2.4 GHz)");
        b.Pass(wifi_password, "Password");
        b.LED(wifi_connected, "Status");

        if (b.Button("Reconnect")) {
            LOG_LN(sets::Logger::info() + "Reconnecting STA...");
            db->update(); // сохраняем БД не дожидаясь таймаута
            WiFiConnector.connect(db->get(wifi_ssid), db->get(wifi_password));
        }

        b.Log(logger_name, logger);
    }

    MQTTClient.buildUI(b);
    EffectController.buildUI(b);
}
