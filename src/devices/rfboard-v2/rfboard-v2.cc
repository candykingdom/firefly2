#include <Arduino.h>

// The onboard addressable LED, which then feeds the external LED connector
constexpr int kPinLedInternal = PA4; 
constexpr int kPinRedLed = PB1;
constexpr int kPinSw1 = PB6;
constexpr int kPinSw2 = PB7;

constexpr int kPin5vDivided = PA0;
constexpr int kPinCc1 = PA1;
constexpr int kPinCc2 = PB2;
constexpr int kPinVibrationSensor = PA8;

void setup() {
  pinMode(kPinLedInternal, OUTPUT);
  pinMode(kPinRedLed, OUTPUT);
  pinMode(kPinSw1, INPUT_PULLUP);
  pinMode(kPinSw2, INPUT_PULLUP);

  pinMode(kPin5vDivided, INPUT_ANALOG);
  pinMode(kPinCc1, INPUT_ANALOG);
  pinMode(kPinCc2, INPUT_ANALOG);
  pinMode(kPinVibrationSensor, INPUT);
}

void loop() {
  digitalWrite(kPinRedLed, (millis() / 100) % 2);
}
