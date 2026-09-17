#pragma once
// Everything the network layer shares: database keys the panel binds to, the
// lamp's name, and the shape of its MQTT topics.

#include <Arduino.h>
#include <StringUtils.h>

namespace net
{
    // Keys the web panel binds widgets to. The panel reads and writes these in
    // the database directly; the code reads them back when it needs a value.
    constexpr size_t kWifiSsid = "wifi_ssid"_h;
    constexpr size_t kWifiPass = "wifi_pass"_h;
    constexpr size_t kMqttHost = "mqtt_host"_h;
    constexpr size_t kMqttPort = "mqtt_port"_h;
    constexpr size_t kMqttUser = "mqtt_user"_h;
    constexpr size_t kMqttPass = "mqtt_pass"_h;
    constexpr size_t kLampName = "lamp_name"_h;
    constexpr size_t kPairName = "pair_name"_h;
    constexpr size_t kPanelPass = "panel_pass"_h;

    // Ids of panel widgets that are not bound to the database.
    constexpr size_t kIdPower = "ui_power"_h;
    constexpr size_t kIdBrightness = "ui_brightness"_h;
    constexpr size_t kIdEffect = "ui_effect"_h;
    constexpr size_t kIdWifiLed = "ui_wifi_led"_h;
    constexpr size_t kIdMqttLed = "ui_mqtt_led"_h;
    constexpr size_t kIdInfo = "ui_info"_h;
    constexpr size_t kIdMem = "ui_mem"_h;
    constexpr size_t kIdLog = "ui_log"_h;

    // "lamp-a4f2" from the MAC until the owner names it in the panel.
    const String& lampName();

    const String& pairName();

    void refreshNames();

    // "lamp/<name>/<suffix>"
    String topicOf(const String& name, const char* suffix);

    inline String topic(const char* suffix) { return topicOf(lampName(), suffix); }
}
