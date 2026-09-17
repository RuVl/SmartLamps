// The settings panel, on GyverLibs Settings.
//
// Every widget binds to a database key - the lamp's own state, the effect
// parameters, WiFi and MQTT alike. Settings then keeps open panels in sync
// through its own rate-limited update channel, and the code only reacts when
// a widget reports a change. There are no unsolicited state pushes from here:
// on the ESP8266 a burst of WebSocket sends from the loop races the TCP ack
// path and crashes in AsyncWebSocketClient::_onAck (seen on hardware -
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

#ifdef LAMP_DEBUG_HTTP
namespace hal
{
    void ledDebugHold(int level); // hal/esp8266/LedDriverDma.cpp
    String ledDebugStats();
}

namespace
{
    // Poor man's logic analyser: sample one input in a tight loop with
    // interrupts off and record the cycle count of every edge. 80 MHz gives
    // 12.5 ns per cycle; the loop itself costs ~50 ns per sample.
    constexpr uint8_t kLaPin = 14; // D5, fed from DIN through a 10k/10k divider
    constexpr uint8_t kLaEdges = 48;
    String g_laResult;

    void captureEdges(uint32_t windowUs)
    {
        uint32_t stamps[kLaEdges];
        uint8_t levels[kLaEdges];
        uint8_t n = 0;
        pinMode(kLaPin, INPUT);
        noInterrupts();
        const uint32_t start = ESP.getCycleCount();
        const uint32_t limit = windowUs * 80;
        uint8_t last = uint8_t((GPIO_REG_READ(GPIO_IN_ADDRESS) >> kLaPin) & 1);
        while (ESP.getCycleCount() - start < limit && n < kLaEdges)
        {
            const uint8_t now = uint8_t((GPIO_REG_READ(GPIO_IN_ADDRESS) >> kLaPin) & 1);
            if (now != last)
            {
                stamps[n] = ESP.getCycleCount();
                levels[n] = now;
                ++n;
                last = now;
            }
        }
        interrupts();

        g_laResult = String(F("la: "));
        g_laResult += n;
        g_laResult += F(" edges in ");
        g_laResult += windowUs;
        g_laResult += F(" us; level after edge / ns held: ");
        for (uint8_t i = 0; i + 1 < n; ++i)
        {
            g_laResult += levels[i] ? 'H' : 'L';
            g_laResult += (stamps[i + 1] - stamps[i]) * 25 / 2; // cycles -> ns
            g_laResult += ' ';
        }
        if (n == 0) g_laResult += F("(no edges)");
    }

    // Faster variant: raw samples into RAM, run lengths worked out afterwards.
    // ~6 cycles per sample, so a 312 ns pulse spans ~4 samples.
    void captureRaw()
    {
        constexpr uint16_t kSamples = 2048;
        uint8_t* buf = static_cast<uint8_t*>(malloc(kSamples));
        if (buf == nullptr) { g_laResult = F("lb: no memory"); return; }
        pinMode(kLaPin, INPUT);
        noInterrupts();
        const uint32_t t0 = ESP.getCycleCount();
        for (uint16_t i = 0; i < kSamples; ++i)
            buf[i] = uint8_t(GPIO_REG_READ(GPIO_IN_ADDRESS) >> kLaPin);
        const uint32_t t1 = ESP.getCycleCount();
        interrupts();

        const uint32_t nsPerSample = (t1 - t0) * 25 / 2 / kSamples;
        g_laResult = String(F("lb: "));
        g_laResult += nsPerSample;
        g_laResult += F(" ns/sample; runs: ");
        uint16_t runs = 0, len = 1;
        for (uint16_t i = 1; i < kSamples && runs < 60; ++i)
        {
            if ((buf[i] & 1) == (buf[i - 1] & 1)) { ++len; continue; }
            g_laResult += (buf[i - 1] & 1) ? 'H' : 'L';
            g_laResult += len * nsPerSample;
            g_laResult += ' ';
            len = 1;
            ++runs;
        }
        if (runs == 0) g_laResult += (buf[0] & 1) ? F("flat H") : F("flat L");
        free(buf);
    }
}
#endif

namespace net::web
{
    namespace
    {
        // A slider is echoed to the lamp no more often than this. 250 ms is what the
        // previous firmware settled on after crashes at higher rates.
        constexpr uint16_t kSliderThrottleMs = 250;
        constexpr uint32_t kLogPushMs = 1000;

#ifdef LAMP_DEBUG_HTTP
        // Debug builds take commands over HTTP: the serial RX pin is the strip's
        // data line on the ESP8266, so there is no way to type at the board.
        struct DebugSettings final : SettingsAsyncWS
        {
            using SettingsAsyncWS::SettingsAsyncWS;
            AsyncWebServer& http() { return server; }
        };
        DebugSettings settings("SmartLamp", &hal::database());
        char g_debugCmd[32] = {}; // one command at a time, applied from tick()
#else
        SettingsAsyncWS settings("SmartLamp", &hal::database());
#endif
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

        bool trimVal(size_t key)
        {
            String s = hal::database().get(key).toString();
            s.trim();
            return hal::database().GyverDB::update(key, s);
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
            if (b.Input(kWifiSsid, "Сеть (только 2,4 ГГц)")) trimVal(kWifiSsid);
            if (b.Pass(kWifiPass, "Пароль")) trimVal(kWifiPass);
            b.LED(kIdWifiLed, "Подключено", wifi::connected());
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
            b.Label(kIdInfo, "Состояние", info);

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
            b.Label(kIdMem, "Память, байт", mem);

            if (b.Button("Применить имя и перезагрузить")) g_pending = Pending::Restart;
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

#ifdef LAMP_DEBUG_HTTP
        // GET /dbg?c=<cmd>: p0|p1 power, b<n> brightness %, e<n> effect index,
        // x<key>=<v> effect param, g0|g1|gr data pin DC low/high/resume,
        // r restart, s state. The reply is the state before the
        // command; the command itself runs in tick(), out of the sys context.
        settings.http().on("/dbg", HTTP_GET, [](AsyncWebServerRequest* r) {
            if (r->hasParam("c"))
                strlcpy(g_debugCmd, r->getParam("c")->value().c_str(), sizeof(g_debugCmd));
            app::Lamp& lamp = app::lamp();
            String out;
            out += F("power="); out += lamp.isOn();
            out += F(" brightness="); out += lamp.brightness();
            out += F(" effect="); out += lamp.effectIndex(); out += ':'; out += lamp.effectName();
            out += F(" fps="); out += lamp.fps();
            out += F(" heap="); out += ESP.getFreeHeap();
            out += hal::ledDebugStats();
            if (!g_laResult.isEmpty()) { out += '\n'; out += g_laResult; }
            out += F(" cmd="); out += g_debugCmd; out += '\n';
            r->send(200, "text/plain", out);
        });
#endif
    }

#ifdef LAMP_DEBUG_HTTP
    namespace
    {
        void runDebugCmd()
        {

            if (g_debugCmd[0] == 0) return;
            app::Lamp& lamp = app::lamp();
            const char* c = g_debugCmd;
            Serial.printf("dbg: %s\n", c);
            switch (c[0])
            {
                case 'p': lamp.setPower(c[1] == '1'); break;
                case 'r': ESP.restart(); break;
                case 'b': lamp.setBrightness(uint8_t(atoi(c + 1))); break;
                case 'e': lamp.selectEffect(uint16_t(atoi(c + 1))); break;
                // la<us>: capture edges on D5 for <us> microseconds (default 200).
                case 'l':
                    if (c[1] == 'b') captureRaw();
                    else captureEdges(c[2] ? uint32_t(atoi(c + 2)) : 200);
                    break;
                // g0 / g1 static level, gw<hz> square wave, gr resume DMA.
                case 'g':
                    if (c[1] == 'r') hal::ledDebugHold(-1);
                    else if (c[1] == 'w') hal::ledDebugHold(atoi(c + 2));
                    else hal::ledDebugHold(c[1] == '1' ? 1 : 0);
                    break;
                case 'x':
                {
                    char* eq = strchr(g_debugCmd, '=');
                    if (eq != nullptr)
                    {
                        *eq = 0;
                        lamp.setParam(c + 1, int16_t(atoi(eq + 1)));
                    }
                    break;
                }
                default: break;
            }
            Serial.printf("state: power=%d brightness=%u effect=%s\n",
                          lamp.isOn(), lamp.brightness(), lamp.effectName());
            g_debugCmd[0] = 0;
        }
    }
#endif

    void tick()
    {
        const uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < g_minFreeHeap) g_minFreeHeap = freeHeap;

        settings.tick();
#ifdef LAMP_DEBUG_HTTP
        runDebugCmd();
#endif

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
        if (now - last < kLogPushMs) return false;
        last = now;
        settings.updater().update(kIdLog, log());
        return true;
    }
}
