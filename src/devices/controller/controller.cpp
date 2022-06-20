#include <FastLED.h>

#include "Arduino.h"

const uint16_t kLedCount = 42;
CRGB leds[kLedCount];

void setup() { FastLED.addLeds<NEOPIXEL, PB15>(leds, kLedCount); }

bool output = false;
void loop() {
  FastLED.showColor(CHSV(millis() / 10, 255, 64));

  delay(10);
}