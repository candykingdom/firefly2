#include "FakeLedManager.hpp"

FakeLedManager::FakeLedManager(const uint8_t numLeds) : LedManager(numLeds) {
  leds = new CRGB[numLeds];
}

void FakeLedManager::SetGlobalColor(CRGB rgb) {
  for (uint8_t i = 0; i < numLeds; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint8_t ledIndex) { return leds[ledIndex]; }

void FakeLedManager::SetLed(uint8_t ledIndex, CRGB *const rgb) {
  leds[ledIndex] = *rgb;
}

void FakeLedManager::WriteOutLeds() {}
