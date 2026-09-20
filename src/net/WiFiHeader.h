#pragma once
// The WiFi class comes from a different header on each core.
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
