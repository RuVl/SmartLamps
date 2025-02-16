#ifndef TIMER_H
#define TIMER_H

#include <core_esp8266_features.h>

struct Timer {
  // указать период, опционально статус (запущен/не запущен)
  explicit Timer(uint32_t prd, bool state = true) {
    this->state = state;
    setPeriod(prd);
    tmr = millis();
  }

  // сменить период
  void setPeriod(uint32_t period) {
    this->prd = period;
  }

  // перезапустить
  void restart() {
    tmr = millis();
    state = true;
  }

  // время с рестарта вышло
  [[nodiscard]] bool elapsed() const {
    return !state || check();
  }

  // периодический таймер
  bool period() {
    if (state && check()) {
      restart();
      return true;
    }
    return false;
  }

  [[nodiscard]] bool check() const {
    return millis() - tmr >= prd;
  }

  uint32_t tmr, prd{};
  bool state = true;
};

#endif //TIMER_H
