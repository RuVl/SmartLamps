// The settings panel, on GyverLibs Settings.
//
// Every widget binds to a database key - the lamp's own state, the effect
// parameters, WiFi and MQTT alike. Settings then keeps open panels in sync
// through its own rate-limited update channel, and the code only reacts when
// a widget reports a change. The few unsolicited pushes that do originate here
// (the log, a rebuild after an effect change) share one throttle: on the
// ESP8266 a burst of WebSocket sends from the loop races the TCP ack path and
// crashes in AsyncWebSocketClient::_onAck (seen on hardware - use-after-free,
// Exception 28 in sys context).
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
#include "MemLog.h"
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
        // Minimum gap between any two pushes we start from loop().
        constexpr uint32_t kPushGapMs = 250;

        SettingsAsyncWS settings("SmartLamp", &hal::database());
        String g_effectOptions; // "Огонь;Радуга;…", built once from the registry

        // The build callbacks run inside the WebSocket receive callback, i.e. in
        // the SDK's sys context. A flash write, a WiFi mode change or a reset
        // from there is not safe on the ESP8266, so a button only records the
        // request and tick() carries it out from loop().
        enum class Pending : uint8_t
        {
            None,
            WifiReconnect,
            MqttReconnect,
            Restart
        };
        Pending g_pending = Pending::None;

        // Heap low-water mark, sampled every loop. On 80 KB the peak is what
        // decides whether the panel, MQTT and the strip fit together.
        uint32_t g_minFreeHeap = UINT32_MAX;

        uint32_t g_lastPushMs = 0;
        bool g_reloadPending = false;

        // Claims the right to send one unsolicited WebSocket message now.
        bool pushSlot()
        {
            const uint32_t now = millis();
            if (now - g_lastPushMs < kPushGapMs) return false;
            g_lastPushMs = now;
            return true;
        }

        bool trimVal(size_t key)
        {
            return hal::database().GyverDB::update(key, hal::dbString(key));
        }

        // One widget per Param::Kind. Returns true when the panel changed the value.
        bool paramWidget(sets::Builder& b, size_t id, const core::Param& p)
        {
            switch (p.kind())
            {
            case core::Param::Kind::Hue:
                {
                    // The slider is tinted with the current hue so the user sees
                    // what they are picking. Settings cannot recolour a widget
                    // live, so the tint follows only on the next panel build.
                    const CRGB c = CHSV(uint8_t(p.get()), 255, 255);
                    const uint32_t rgb = (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
                    return b.Slider(id, p.label(), 0, 255, 1, "", nullptr, rgb);
                }
            case core::Param::Kind::Switch: return b.Switch(id, p.label());
            case core::Param::Kind::Select: return b.Select(id, p.label(), p.options());
            case core::Param::Kind::Slider: break;
            }
            return b.Slider(id, p.label(), p.min(), p.max(), 1);
        }

        void buildLampMenu(sets::Builder& b)
        {
            app::Lamp& lamp = app::lamp();
            GyverDBFile& db = hal::database();
            sets::Menu menu(b, "Лампа");

            if (b.Switch(app::keys::kPower, "Питание"))
                lamp.setPower(db.get(app::keys::kPower).toBool());

            if (b.Slider(app::keys::kBrightness, "Яркость", 0, 100, 1, "%"))
                lamp.setBrightness(uint8_t(db.get(app::keys::kBrightness).toInt()));

            // No b.reload() here: the new effect is built only after the 300 ms
            // fade-out, and a rebuild now would still list the old parameters.
            // tick() reloads the panel once Lamp reports the activation.
            if (b.Select(app::keys::kEffect, "Эффект", g_effectOptions))
                lamp.selectEffect(uint16_t(db.get(app::keys::kEffect).toInt()));

            // Everything below comes from the effect's Param declarations. The keys
            // are the same ones Lamp reads the parameters back from at boot.
            if (lamp.hasParams())
            {
                sets::Group group(b, "Параметры");
                const char* effect = lamp.effectName();
                lamp.forEachParam([&](core::Param& p)
                {
                    const size_t id = hal::paramKey(effect, p.key());
                    // A Switch lands in the database as a bool; toInt() reads it as 0/1.
                    if (paramWidget(b, id, p)) lamp.setParam(p.key(), int16_t(db.get(id).toInt()));
                });
                memlog::sampleStack();
            }
        }

        void buildWifiMenu(sets::Builder& b)
        {
            sets::Menu menu(b, "WiFi");
            if (b.Input(kWifiSsid, "Сеть (только 2,4 ГГц)")) trimVal(kWifiSsid);
            if (b.Pass(kWifiPass, "Пароль")) trimVal(kWifiPass);
            b.LED(kIdWifiLed, "Подключено", wifi::connected());
            // Paragraph, not Label: a Label is one line on the right and the
            // status with an IP, RSSI and a reason does not fit it.
            b.Paragraph(kIdWifiState, "Состояние", wifi::status());
            b.Log(kIdWifiLog, wifiLog());
            if (b.Button("Переподключить")) g_pending = Pending::WifiReconnect;
        }

        void buildMqttMenu(sets::Builder& b)
        {
            sets::Menu menu(b, "MQTT");
            if (b.Input(kMqttHost, "Сервер")) trimVal(kMqttHost);
            b.Number(kMqttPort, "Порт", nullptr, 1, 65535);
            if (b.Input(kMqttUser, "Пользователь")) trimVal(kMqttUser);
            if (b.Pass(kMqttPass, "Пароль")) trimVal(kMqttPass);
            b.LED(kIdMqttLed, "Подключено", mqtt::connected());
            b.Paragraph(kIdMqttState, "Состояние", mqtt::status());
            b.Log(kIdMqttLog, mqttLog());
            if (b.Button("Переподключить")) g_pending = Pending::MqttReconnect;
        }

        void buildSystemMenu(sets::Builder& b)
        {
            sets::Menu menu(b, "Система");
            if (b.Input(kLampName, "Имя лампы")) trimVal(kLampName);
            if (b.Input(kPairName, "Парная лампа")) trimVal(kPairName);
            if (b.Pass(kPanelPass, "Пароль панели")) trimVal(kPanelPass);

            String info;
            info += F("IP ");
            info += WiFi.localIP().toString();
            info += F(" · RSSI ");
            info += WiFi.RSSI();
            info += F(" · ");
            info += app::lamp().fps();
            info += F(" к/с");
            b.Paragraph(kIdInfo, "Состояние", info);

            String mem;
            mem += F("свободно ");
            mem += ESP.getFreeHeap();
            mem += F(" · минимум ");
            mem += g_minFreeHeap;
            mem += F(" · блок ");
#ifdef ESP8266
            mem += ESP.getMaxFreeBlockSize();
            mem += F(" · стек loop ");
            mem += ESP.getFreeContStack();
#else
            mem += ESP.getMaxAllocHeap();
#endif
            b.Paragraph(kIdMem, "Память, байт", mem);

            if (b.Button("Применить имя и перезагрузить")) g_pending = Pending::Restart;
            // The full journal lives here only; the WiFi and MQTT pages get
            // their own short ones - every Log widget is its buffer's size in
            // the page and in each pushLog() packet, see docs/memory.md.
            b.Log(kIdLog, log());
            memlog::sampleStack();
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

        // init() writes only into a fresh database; afterwards the panel's own field wins.
        GyverDBFile& db = hal::database();
        db.init(kPanelPass, LAMP_PANEL_PASS);
        const String pass = db.get(kPanelPass).toString();
        if (!pass.isEmpty()) settings.setPass(pass);

        settings.setProjectInfo("SmartLamp", "https://github.com/RuVl/SmartLamps");
        settings.config.sliderTout = kSliderThrottleMs;
        settings.onBuild(build);
        settings.begin();

    }

    void tick()
    {
        const uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < g_minFreeHeap) g_minFreeHeap = freeHeap;

        settings.tick();

        if (app::lamp().consumeActivated()) g_reloadPending = true;
        if (g_reloadPending && pushSlot())
        {
            g_reloadPending = false;
            settings.reload(); // sends only to a focused panel, else defers to its next request
        }

        const Pending pending = g_pending;
        g_pending = Pending::None;
        if (pending == Pending::None) return;

        hal::storage().flush(); // the panel edited the database; keep the change
        switch (pending)
        {
            case Pending::WifiReconnect: wifi::reconnect(); break;
            case Pending::MqttReconnect: mqtt::reconnect(); break;
            case Pending::Restart: ESP.restart(); break;
            case Pending::None: break;
        }
    }

    bool pushLog()
    {
        static uint32_t last = 0;
        const uint32_t now = millis();
        if (now - last < kLogPushMs || !pushSlot()) return false;
        last = now;
        // The Logger overload streams the journal straight into the packet: no
        // 1 KB String copy on the heap for every push.
        settings.updater()
            .update(kIdLog, log())
            .update(kIdWifiLog, wifiLog())
            .update(kIdMqttLog, mqttLog())
            .update(kIdWifiState, wifi::status())
            .update(kIdMqttState, mqtt::status())
            .update(kIdWifiLed, wifi::connected())
            .update(kIdMqttLed, mqtt::connected());
        return true;
    }

    bool focused() { return settings.focused(); }
}
