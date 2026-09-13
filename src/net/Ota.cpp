#include "Ota.h"

#include <ArduinoOTA.h>

#include "Config.h"
#include "Log.h"
#include "app/Lamp.h"
#include "hal/Storage.h"

namespace net::ota {
    namespace {
        bool g_started = false;
        bool g_active = false;
    } // namespace

    void begin() {
        ArduinoOTA.setHostname(lampName().c_str());
        ArduinoOTA.onStart([] {
            g_active = true;
            app::lamp().setStatus(app::Status::Updating);
            // Whatever was pending must reach flash before the partition is
            // rewritten underneath the filesystem driver.
            hal::storage().flush();
            logInfo(F("OTA: начало обновления"));
        });
        ArduinoOTA.onEnd([] {
            g_active = false;
            logInfo(F("OTA: готово, перезагрузка"));
        });
        ArduinoOTA.onError([](ota_error_t e) {
            g_active = false;
            logError(String(F("OTA: ошибка ")) + int(e));
        });
    }

    void start() {
        if (g_started) return;
        g_started = true;
        ArduinoOTA.begin();
        logInfo(F("OTA: слушаю"));
    }

    void tick() {
        if (g_started) ArduinoOTA.handle();
    }

    bool active() { return g_active; }
} // namespace net::ota
