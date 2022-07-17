#include <Arduino.h>
#include <FastLED.h>

const int kLedPin = PB0;
const int kNeopixelPin = PA12;
CRGB leds[2];

void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, HIGH);

  FastLED.addLeds<NEOPIXEL, kNeopixelPin>(leds, 2);
}

void loop() {
  FastLED.showColor(CHSV(millis() / 10, 255, 255));
  digitalWrite(kLedPin, (millis() / 200) % 2);
  delay(10);
}