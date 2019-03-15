#include "FastLedManager.hpp"

FastLedManager::FastLedManager(const uint8_t numLeds) : LedManager(numLeds) {
  leds = new CRGB[numLeds];
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, numLeds);
  FastLED.showColor(CRGB(0, 0, 0));
}

void FastLedManager::SetLed(uint8_t ledIndex, CRGB &rgb) {
  leds[ledIndex] = rgb;
}

void FastLedManager::WriteOutLeds() { FastLED.show(); }
