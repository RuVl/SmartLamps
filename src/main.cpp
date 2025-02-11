#include <Arduino.h>

void setup() {
  // Serial.begin(9600);         // запускаем сериал для отладки
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}