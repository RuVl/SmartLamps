#include "Wifi.h"

#include <WiFiConnector.h>

extern "C" {
#include <user_interface.h>
}

#include "Config.h"
#include "Log.h"
#include "Mqtt.h"
#include "Ota.h"
#include "hal/Database.h"
#include "hal/Mailbox.h"

namespace net::wifi
{
    namespace
    {
        // Access point password. Nobody wants an open AP in the block of flats; the
        // name is the lamp's own, so two lamps never collide.
        constexpr const char* kApPass = LAMP_AP_PASS; // from secrets.ini
        constexpr uint16_t kConnectTimeoutS = 20;
        // If STA is lost for this long the AP comes back, so the lamp stays
        // reachable when the router is down and not only when it never came up.
        constexpr uint32_t kReopenApAfterMs = 30000;

        uint32_t g_lostSince = 0;

        hal::Mailbox<uint8_t> g_reason;
        uint8_t g_lastLoggedReason = 0;
        String g_lastError; // what the panel shows next to the LED
        bool g_connecting = false;
        bool g_apClosing = false;

        // WiFi.mode() waits for the SDK to finish the switch: esp_delay() in
        // the loop() context, 100 ms at the least and up to a second, and no
        // frame is rendered meanwhile. The SDK call alone returns at once, and
        // nothing here needs the new mode in force before the next tick.
        void switchMode(WiFiMode_t m) { wifi_set_opmode_current(uint8_t(m)); }

        const __FlashStringHelper* reasonText(uint8_t code)
        {
            // 802.11 reason codes plus the Espressif 200+ range; the words are
            // what the owner needs to hear.
            switch (code)
            {
            case 2: // AUTH_EXPIRE
            case 202: // AUTH_FAIL
            case 23: // 802_1X_AUTH_FAILED
                return F("роутер отверг пароль");
            case 15: // 4WAY_HANDSHAKE_TIMEOUT
            case 204: // HANDSHAKE_TIMEOUT
                return F("таймаут handshake - скорее всего неверный пароль");
            case 201: return F("сеть не найдена (нет 2,4 ГГц или другое имя)");
            case 200: return F("пропал сигнал роутера");
            case 203: return F("роутер не принял ассоциацию");
            case 8: return F("отключились сами");
            case 3:
            case 4: return F("роутер закрыл соединение");
            default: return F("неизвестная причина");
            }
        }

        void installReasonHook()
        {
            // The handler object must outlive the registration, hence static.
            static WiFiEventHandler handler = WiFi.onStationModeDisconnected(
                [](const WiFiEventStationModeDisconnected& e) { g_reason.set(uint8_t(e.reason)); });
        }

        void onConnected()
        {
            g_connecting = false;
            g_lastError = "";
            logInfo(String(F("WiFi: подключено, IP ")) + WiFi.localIP().toString() +
                    F(", RSSI ") + WiFi.RSSI() + F(" дБм"));
            ota::start();
            mqtt::reconnect();
        }

        void onFailed()
        {
            g_connecting = false;
            if (g_lastError.isEmpty()) g_lastError = F("таймаут подключения");
            logWarn(String(F("WiFi: не удалось подключиться (")) + g_lastError +
                    F("), точка доступа открыта"));
        }
    }

    void begin()
    {
        GyverDBFile& db = hal::database();
        db.init(kWifiSsid, "");
        db.init(kWifiPass, "");

        // Persistent, so the SDK keeps the station config - and the WPA2 key
        // it derives from the passphrase - in its own flash sector. Deriving
        // that key is 4096 rounds of HMAC-SHA1 in the sys context, close to a
        // second with no frame on the matrix; with the config stored it is
        // paid once, when the credentials change, not on every boot.
        WiFi.persistent(true);
        // AP+STA from the start so the panel is reachable while STA is trying.
        WiFi.mode(WIFI_AP_STA);
        // Station-only mode lets the SDK open modem sleep ("pm open" in the
        // boot log); with the I2S DMA feeding the strip that ends in a hard
        // hang and a hardware-watchdog reset seconds after the AP is closed.
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        WiFi.hostname(lampName());

        WiFiConnector.onConnect(onConnected);
        WiFiConnector.onError(onFailed);
        WiFiConnector.setName(lampName());
        WiFiConnector.setPass(kApPass);
        WiFiConnector.setTimeout(kConnectTimeoutS);
        // The AP goes away once STA is up and comes back if STA is lost, so the
        // panel is always reachable one way or the other. Closed from tick(),
        // not by WiFiConnector: its closeAP() goes through WiFi.mode() and its
        // wait, right when the lamp is showing an effect.
        WiFiConnector.closeAP(false);
        installReasonHook();

        reconnect();
    }

    void reconnect()
    {
        GyverDBFile& db = hal::database();
        const String ssid = db.get(kWifiSsid).toString();
        if (ssid.isEmpty())
        {
            logInfo(F("WiFi: сеть не задана, работаю точкой доступа"));
            WiFiConnector.connect("");
            return;
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            // Otherwise WiFiConnector sees the old WL_CONNECTED on its first
            // tick and reports the new attempt as done before it started.
            logInfo(F("WiFi: рву текущее соединение"));
            WiFi.disconnect();
        }
        g_lastError = "";
        g_connecting = true;
        logInfo(String(F("WiFi: подключаюсь к ")) + ssid);
        WiFiConnector.connect(ssid, db.get(kWifiPass).toString());
    }

    void tick()
    {
        uint8_t code = 0;
        if (g_reason.take(code))
        {
            g_lastError = String(reasonText(code)) + F(" [") + code + ']';
            // The SDK retries on its own and repeats the same reason every few
            // seconds; one line per distinct reason is what the log needs.
            if (code != g_lastLoggedReason)
            {
                g_lastLoggedReason = code;
                logWarn(String(F("WiFi: отключено: ")) + g_lastError);
            }
        }

        WiFiConnector.tick();

        if (connected())
        {
            g_lostSince = 0;
            g_lastLoggedReason = 0; // the next drop deserves a line again
            if (accessPointUp() && !g_apClosing)
            {
                g_apClosing = true;
                switchMode(WIFI_STA);
            }
            return;
        }
        g_apClosing = false;
        if (g_lostSince == 0) g_lostSince = millis();
        if (!accessPointUp() && millis() - g_lostSince >= kReopenApAfterMs)
        {
            switchMode(WIFI_AP_STA);
            WiFi.softAP(lampName().c_str(), kApPass);
            logWarn(F("WiFi: сеть потеряна, точка доступа открыта снова"));
        }
    }

    bool connected() { return WiFiConnector.connected(); }

    String status()
    {
        if (connected())
        {
            return String(F("подключено, IP ")) + WiFi.localIP().toString() + F(", RSSI ") +
                   WiFi.RSSI() + F(" дБм");
        }
        String s = g_connecting ? String(F("подключаюсь…")) : String(F("не подключено"));
        if (!g_lastError.isEmpty()) s += String(F(" · ")) + g_lastError;
        if (accessPointUp()) s += String(F(" · точка доступа ")) + lampName();
        return s;
    }

    bool accessPointUp() { return (WiFi.getMode() & WIFI_AP) != 0; }
}
