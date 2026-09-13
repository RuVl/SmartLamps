#pragma once

namespace net::ota {

void begin();
void start();  // once WiFi is up; safe to call repeatedly
void tick();
bool active();

}  // namespace net::ota
