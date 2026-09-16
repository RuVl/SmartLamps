// The settings panel, on GyverLibs Settings.
//
// Every widget binds to a database key — the lamp's own state, the effect
// parameters, WiFi and MQTT alike. Settings then keeps open panels in sync
// through its own rate-limited update channel, and the code only reacts when
// a widget reports a change. There are no unsolicited state pushes from here:
// on the ESP8266 a burst of WebSocket sends from the loop races the TCP ack
// path and crashes in AsyncWebSocketClient::_onAck (seen on hardware —
// use-after-free, Exception 28 in sys context).
//
// The parameter sliders are generated from the active effect's Param list, so
// a new effect appears in the panel with no UI code at all.

#include "WebUi.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SettingsAsyncWS.h>
#pragma GCC diagnostic pop

#include "Config.h"
#include "Log.h"
#include "Mqtt.h"
#include "Wifi.h"
#include "app/Keys.h"
#include "app/Lamp.h"
#include "core/Registry.h"
#include "hal/Database.h"
#include "hal/Storage.h"

namespace net::web
{
    namespace
    {
        // A slider is echoed to the lamp no more often than this. 250 ms is what the
        // previous firmware settled on after crashes at higher rates.
        constexpr uint16_t kSliderThrottleMs = 250;
        constexpr uint32_t kLogPushMs = 1000;

        SettingsAsyncWS settings("SmartLamp", &hal::database());
        String g_effectOptions; // "Огонь;Радуга;…", built once from the registry

        void buildLampMenu(sets::Builder& b)
        {
            app::Lamp& lamp = app::lamp();
            GyverDBFile& db = hal::database();
            sets::Menu menu(b, "Лампа");

            if (b.Switch(app::keys::kPower, "Питание"))
                lamp.setPower(db.get(app::keys::kPower).toBool());

            if (b.Slider(app::keys::kBrightness, "Яркость", 0, 100, 1, "%"))
                lamp.setBrightness(uint8_t(db.get(app::keys::kBrightness).toInt()));

            if (b.Select(app::keys::kEffect, "Эффект", g_effectOptions))
            {
                lamp.selectEffect(uint16_t(db.get(app::keys::kEffect).toInt()));
                b.reload(); // the parameter sliders below belong to the new effect
            }

            // Everything below comes from the effect's Param declarations. The keys
            // are the same ones Lamp reads the parameters back from at boot.
            if (lamp.params() != nullptr)
            {
                sets::Group group(b, "Параметры");
                for (core::Param* p = lamp.params(); p != nullptr; p = p->next())
                {
                    const size_t id = hal::paramKey(lamp.effectName(), p->key());
                    if (b.Slider(id, p->label(), p->min(), p->max(), 1))
                        lamp.setParam(p->key(), int16_t(db.get(id).toInt()));
                }
            }
        }

        void buildWifiMenu(sets::Builder& b)
        {
            sets::Menu menu(b, "WiFi");
            b.Input(kWifiSsid, "Сеть (только 2,4 ГГц)");
            b.Pass(kWifiPass, "Пароль");
            b.LED(kIdWifiLed, "Подключено", wifi::connected());
            if (b.Button("Переподключить"))
            {
                hal::storage().flush();
                wifi::reconnect();
            }
        }

        void buildMqttMenu(sets::Builder& b)
        {
            sets::Menu menu(b, "MQTT");
            b.Input(kMqttHost, "Сервер");
            b.Number(kMqttPort, "Порт", nullptr, 1, 65535);
            b.Input(kMqttUser, "Пользователь");
            b.Pass(kMqttPass, "Пароль");
            b.LED(kIdMqttLed, "Подключено", mqtt::connected());
            if (b.Button("Переподключить"))
            {
                hal::storage().flush();
                mqtt::reconnect();
            }
        }

        void buildSystemMenu(sets::Builder& b)
        {
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

            if (b.Button("Применить имя и перезагрузить"))
            {
                hal::storage().flush();
                ESP.restart();
            }
            b.Log(kIdLog, log());
        }

        void build(sets::Builder& b)
        {
            buildLampMenu(b);
            buildWifiMenu(b);
            buildMqttMenu(b);
            buildSystemMenu(b);
        }
    }

    void begin()
    {
        for (core::EffectInfo* e = core::Registry::head(); e != nullptr; e = e->next)
        {
            if (!g_effectOptions.isEmpty()) g_effectOptions += ';';
            g_effectOptions += e->name;
        }

        GyverDBFile& db = hal::database();
        db.init(kPanelPass, "");
        const String pass = db.get(kPanelPass).toString();
        if (!pass.isEmpty()) settings.setPass(pass);

        settings.setProjectInfo("SmartLamp", "https://github.com/RuVl/SmartLamps");
        settings.config.sliderTout = kSliderThrottleMs;
        settings.onBuild(build);
        settings.begin();
    }

    void tick() { settings.tick(); }

    bool pushLog()
    {
        static uint32_t last = 0;
        const uint32_t now = millis();
        if (now - last < kLogPushMs) return false;
        last = now;
        settings.updater().update(kIdLog, log());
        return true;
    }
}
