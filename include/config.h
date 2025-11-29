#ifndef CONFIG_H
#define CONFIG_H

// ========== Settings ==========
// ----- Logging -----
// #define DEBUG_ESP_HTTP_SERVER
#define ENABLE_WEB_LOGGING          // Логирование в веб-интерфейс
#define ENABLE_SERIAL_LOGGING       // Логирование в Serial
#define LOG_BUFFER_CAPACITY 1000    // Размер буффера с логами

// ----- FastLED -----
#define FASTLED_USE_PROGMEM 1 // просим библиотеку FASTLED экономить память контроллера на свои палитры
#define FASTLED_INTERNAL
// #define FASTLED_OVERCLOCK 1.1       // Overclocks by 10%

// ----- Matrix -----
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
#define LED_MAX_AMPERAGE 3750       // Лимит по току в миллиамперах, автоматически управляет яркостью

// ----- Button -----
#define BTN_PIN D2

// ----- WiFi -----
#define WIFI_CLOSE_AP false         // Выключать AP после подключения STA
#define WIFI_CONNECTION_TIMEOUT 20
#define WIFI_AP_NAME "RuVlamp"
#define WIFI_AP_PASSWORD "12345678"

// ----- MQTT -----
#define MQTT_KEEPALIVE 15
#define MQTT_SOCKET_TIMEOUT 15

// ----- Web -----
#define ADMIN_PASSWORD "1234" // Password for the web panel
// слишком частые апдейты крашат программу
#define SLIDER_TIMEOUT 250 // Minimum time between updates

// ----- Database -----
#define DB_PATH "/data.db"
// #define ATOMIC_FS_UPDATE


// ========== Other Logic ==========
// ----- LED -----
#define GAMMA 1.5 // gamma-correction for brightness

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
