#include <Arduino.h>

constexpr int kPinRedLed = PB1;

void setup() {
  pinMode(kPinRedLed, OUTPUT);
}

void loop() {
  digitalWrite(kPinRedLed, (millis() / 100) % 2);
}
