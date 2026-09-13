// The settings panel, on GyverLibs Settings.
//
// Widgets for the lamp's own state bind to app::Lamp through its setters; the
// parameter sliders are generated from the active effect's Param list, so a
// new effect appears in the panel with no UI code at all. WiFi, MQTT and
// naming widgets bind straight to database keys and the code reads them back.

#include "WebUi.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SettingsAsyncWS.h>
#pragma GCC diagnostic pop

#include "Config.h"
#include "Log.h"
#include "Mqtt.h"
#include "Wifi.h"
#include "app/Lamp.h"
#include "core/Registry.h"
#include "hal/Database.h"
#include "hal/Storage.h"

namespace net::web {
    namespace {
        // A slider is echoed to the lamp no more often than this; a panel dragging
        // at full rate would otherwise starve the loop.
        constexpr uint16_t kSliderThrottleMs = 100;

        SettingsAsyncWS settings("SmartLamp", &hal::database());
        String g_effectOptions; // "Огонь;Радуга;…", built once from the registry

        void buildLampMenu(sets::Builder &b) {
            app::Lamp &lamp = app::lamp();
            sets::Menu menu(b, "Лампа");

            bool on = lamp.isOn();
            if (b.Switch(kIdPower, "Питание", &on)) lamp.setPower(on);

            int brightness = lamp.brightness();
            if (b.Slider(kIdBrightness, "Яркость", 0, 100, 1, "%", &brightness))
                lamp.setBrightness(uint8_t(brightness));

            int effect = lamp.effectIndex();
            if (b.Select(kIdEffect, "Эффект", g_effectOptions, &effect)) {
                lamp.selectEffect(uint16_t(effect));
                b.reload(); // the parameter sliders below belong to the new effect
            }

            // Everything below comes from the effect's Param declarations.
            if (lamp.params() != nullptr) {
                sets::Group group(b, "Параметры");
                for (core::Param *p = lamp.params(); p != nullptr; p = p->next()) {
                    int value = p->get();
                    const size_t id = hal::paramKey(lamp.effectName(), p->key());
                    if (b.Slider(id, p->label(), p->min(), p->max(), 1, "", &value))
                        lamp.setParam(p->key(), int16_t(value));
                }
            }
        }

        void buildWifiMenu(sets::Builder &b) {
            sets::Menu menu(b, "WiFi");
            b.Input(kWifiSsid, "Сеть (только 2,4 ГГц)");
            b.Pass(kWifiPass, "Пароль");
            b.LED(kIdWifiLed, "Подключено", wifi::connected());
            if (b.Button("Переподключить")) {
                hal::storage().flush();
                wifi::reconnect();
            }
        }

        void buildMqttMenu(sets::Builder &b) {
            sets::Menu menu(b, "MQTT");
            b.Input(kMqttHost, "Сервер");
            b.Number(kMqttPort, "Порт", nullptr, 1, 65535);
            b.Input(kMqttUser, "Пользователь");
            b.Pass(kMqttPass, "Пароль");
            b.LED(kIdMqttLed, "Подключено", mqtt::connected());
            if (b.Button("Переподключить")) {
                hal::storage().flush();
                mqtt::reconnect();
            }
        }

        void buildSystemMenu(sets::Builder &b) {
            sets::Menu menu(b, "Система");
            b.Input(kLampName, "Имя лампы");
            b.Input(kPairName, "Парная лампа");
            b.Pass(kPanelPass, "Пароль панели");

            String info;
            info += F("IP ");
            info += WiFi.localIP().toString();
            info += F(" · RSSI ");
            info += WiFi.RSSI();
            info += F(" · ");
            info += app::lamp().fps();
            info += F(" к/с");
            b.Label(kIdInfo, "Состояние", info);

            if (b.Button("Применить имя и перезагрузить")) {
                hal::storage().flush();
                ESP.restart();
            }
            b.Log(kIdLog, log());
        }

        void build(sets::Builder &b) {
            buildLampMenu(b);
            buildWifiMenu(b);
            buildMqttMenu(b);
            buildSystemMenu(b);
        }
    } // namespace

    void begin() {
        for (core::EffectInfo *e = core::Registry::head(); e != nullptr; e = e->next) {
            if (!g_effectOptions.isEmpty()) g_effectOptions += ';';
            g_effectOptions += e->name;
        }

        GyverDBFile &db = hal::database();
        db.init(kPanelPass, "");
        const String pass = db.get(kPanelPass).toString();
        if (!pass.isEmpty()) settings.setPass(pass);

        settings.setProjectInfo("SmartLamp", "https://github.com/RuVl/SmartLamps");
        settings.config.sliderTout = kSliderThrottleMs;
        settings.onBuild(build);
        settings.begin();
    }

    void tick() { settings.tick(); }

    void pushState() {
        app::Lamp &lamp = app::lamp();
        settings.updater()
                .update(kIdPower, lamp.isOn())
                .update(kIdBrightness, int(lamp.brightness()))
                .update(kIdEffect, int(lamp.effectIndex()))
                .update(kIdWifiLed, wifi::connected())
                .update(kIdMqttLed, mqtt::connected());
    }

    void pushLog() { settings.updater().update(kIdLog, log()); }
} // namespace net::web
