#include "Wifi.h"

#include <WiFiConnector.h>

#include "Config.h"
#include "Log.h"
#include "Mqtt.h"
#include "Ota.h"
#include "hal/Database.h"

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

        void onConnected()
        {
            logInfo(String(F("WiFi: подключено, IP ")) + WiFi.localIP().toString());
            ota::start();
            mqtt::reconnect();
        }

        void onFailed()
        {
            logWarn(F("WiFi: не удалось подключиться, точка доступа открыта"));
        }
    }

    void begin()
    {
        GyverDBFile& db = hal::database();
        db.init(kWifiSsid, "");
        db.init(kWifiPass, "");

        // AP+STA from the start so the panel is reachable while STA is trying.
        WiFi.mode(WIFI_AP_STA);
#ifdef ESP32
        WiFi.setHostname(lampName().c_str());
#else
        WiFi.hostname(lampName());
#endif

        WiFiConnector.onConnect(onConnected);
        WiFiConnector.onError(onFailed);
        WiFiConnector.setName(lampName());
        WiFiConnector.setPass(kApPass);
        WiFiConnector.setTimeout(kConnectTimeoutS);
        // The AP goes away once STA is up and comes back if STA is lost, so the
        // panel is always reachable one way or the other.
        WiFiConnector.closeAP(true);

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
        logInfo(String(F("WiFi: подключаюсь к ")) + ssid);
        WiFiConnector.connect(ssid, db.get(kWifiPass).toString());
    }

    void tick()
    {
        WiFiConnector.tick();

        if (connected())
        {
            g_lostSince = 0;
            return;
        }
        if (g_lostSince == 0) g_lostSince = millis();
        if (!accessPointUp() && millis() - g_lostSince >= kReopenApAfterMs)
        {
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP(lampName().c_str(), kApPass);
            logWarn(F("WiFi: сеть потеряна, точка доступа открыта снова"));
        }
    }

    bool connected() { return WiFiConnector.connected(); }

    bool accessPointUp() { return (WiFi.getMode() & WIFI_AP) != 0; }
}
