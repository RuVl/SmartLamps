#include "Ota.h"

#include <ArduinoOTA.h>

#include "Config.h"
#include "Log.h"
#include "app/Lamp.h"
#include "hal/Storage.h"

namespace net::ota
{
    namespace
    {
        bool g_started = false;
        bool g_active = false;
    }

    void begin()
    {
        ArduinoOTA.setHostname(lampName().c_str());
        ArduinoOTA.onStart([]
        {
            g_active = true;
            app::lamp().setStatus(app::Status::Updating);
            // Whatever was pending must reach flash before the partition is
            // rewritten underneath the filesystem driver.
            hal::storage().flush();
            logInfo(F("OTA: начало обновления"));
        });
        ArduinoOTA.onEnd([]
        {
            g_active = false;
            logInfo(F("OTA: готово, перезагрузка"));
        });
        ArduinoOTA.onError([](ota_error_t e)
        {
            g_active = false;
            logError(String(F("OTA: ошибка ")) + int(e));
        });
    }

    void start()
    {
        if (g_started) return;
        g_started = true;
        ArduinoOTA.setPassword(LAMP_OTA_PASS);
        // No mDNS. It joins the multicast group on every interface, the access
        // point's included, and when the AP goes away right after the station
        // comes up, lwIP's IGMP timer still reports for the deleted interface
        // through a dangling link-output pointer: Exception (0) in sys context
        // on every boot (igmp_tmr -> new_linkoutput, seen on lamp B). OTA by
        // IP address works without it.
#ifdef ESP32
        ArduinoOTA.setMdnsEnabled(false);
        ArduinoOTA.begin();
#else
        ArduinoOTA.begin(false);
#endif
        logInfo(F("OTA: слушаю"));
    }

    void tick()
    {
        if (g_started) ArduinoOTA.handle();
    }

    bool active() { return g_active; }
}
