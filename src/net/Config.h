#pragma once
// Everything the network layer shares: database keys the panel binds to, the
// lamp's name, and the shape of its MQTT topics.

#include <Arduino.h>
#include <StringUtils.h>

namespace net {
    // Keys the web panel binds widgets to. The panel reads and writes these in
    // the database directly; the code reads them back when it needs a value.
    constexpr size_t kWifiSsid = su::SH("wifi_ssid");
    constexpr size_t kWifiPass = su::SH("wifi_pass");
    constexpr size_t kMqttHost = su::SH("mqtt_host");
    constexpr size_t kMqttPort = su::SH("mqtt_port");
    constexpr size_t kMqttUser = su::SH("mqtt_user");
    constexpr size_t kMqttPass = su::SH("mqtt_pass");
    constexpr size_t kLampName = su::SH("lamp_name");
    constexpr size_t kPairName = su::SH("pair_name");
    constexpr size_t kPanelPass = su::SH("panel_pass");

    // Ids of panel widgets that are not bound to the database.
    constexpr size_t kIdPower = su::SH("ui_power");
    constexpr size_t kIdBrightness = su::SH("ui_brightness");
    constexpr size_t kIdEffect = su::SH("ui_effect");
    constexpr size_t kIdWifiLed = su::SH("ui_wifi_led");
    constexpr size_t kIdMqttLed = su::SH("ui_mqtt_led");
    constexpr size_t kIdInfo = su::SH("ui_info");
    constexpr size_t kIdLog = su::SH("ui_log");

    // "lamp-a4f2" from the MAC until the owner names it in the panel.
    const String &lampName();

    const String &pairName();

    void refreshNames();

    // "lamp/<name>/<suffix>"
    String topicOf(const String &name, const char *suffix);

    inline String topic(const char *suffix) { return topicOf(lampName(), suffix); }
} // namespace net
