#ifndef CONFIG_H
#define CONFIG_H

// ========== Settings ==========
// ----- Logging -----
// #define DEBUG_ESP_HTTP_SERVER
#define ENABLE_WEB_LOGGING          // Логирование в веб-интерфейс
#define ENABLE_SERIAL_LOGGING       // Логирование в Serial
#define LOG_BUFFER_CAPACITY 1000    // Размер буффера с логами

// ----- FastLED -----
#define FASTLED_INTERNAL
// #define FASTLED_OVERCLOCK 1.1       // Overclocks by 10%

// ----- МАТРИЦА -----
#define WIDTH 16                    // Ширина матрицы
#define HEIGHT 16                   // Высота матрицы
#define NUM_LEDS WIDTH * HEIGHT

#define LED_PIN D1                  // Пин подключения
#define LED_TYPE WS2812B            // Тип матрицы
#define COLOR_ORDER GRB             // Порядок цветов на ленте

#define MATRIX_TYPE 0               // Тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 0          // Угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний

#define STRIP_DIRECTION 0           // Направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
#define SEGMENTS 1                  // Диодов в одном "пикселе" (для создания матрицы из кусков ленты)

// Optional
#define LED_BRIGHTNESS 40           // Стандартная максимальная яркость (0-255)
#define LED_MAX_AMPERAGE 3000       // Лимит по току в миллиамперах, автоматически управляет яркостью

// ----- WiFi -----
#define WIFI_CLOSE_AP false         // Выключать AP после подключения STA
#define WIFI_CONNECTION_TIMEOUT 20
#define WIFI_AP_NAME "RuVlamp"
// #define WIFI_AP_PASSWORD "12345678"

// ----- MQTT -----
#define MQTT_KEEPALIVE 15
#define MQTT_SOCKET_TIMEOUT 15

// ----- Database -----
#define DB_PATH "/data.db"
#include <GyverDB.h>

DB_KEYS(
    kk,

    wifi_ssid,
    wifi_password,
    wifi_connected, // bool

    mqtt_ip,
    mqtt_port,
    mqtt_user,
    mqtt_password,
    mqtt_topic,
    mqtt_connected // bool
)

// ========== Logic ==========
// ----- Logging -----
#ifdef ENABLE_WEB_LOGGING
#define LOG_WEB(x) logger.print(x)
#define LOG_WEB_LN(x) logger.println(x)
#else
  #define LOG_WEB(x) 
  #define LOG_WEB_LN(x) 
#endif

#ifdef ENABLE_SERIAL_LOGGING
#define LOG_SERIAL(x) Serial.print(x)
#define LOG_SERIAL_LN(x) Serial.println(x)
#else
  #define LOG_SERIAL(x)
  #define LOG_SERIAL_LN(x)
#endif

#define LOG(x) do { LOG_WEB(x); LOG_SERIAL(x); } while(0)
#define LOG_LN(x) do { LOG_WEB_LN(x); LOG_SERIAL_LN(x); } while(0)

#endif //CONFIG_H
