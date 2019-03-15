#include "FakeLedManager.hpp"

FakeLedManager::FakeLedManager(const uint8_t numLeds) : LedManager(numLeds) {
  leds = new CRGB[numLeds];
}

void FakeLedManager::SetLed(uint8_t ledIndex, CRGB &rgb) {
  leds[ledIndex] = rgb;
}

void FakeLedManager::WriteOutLeds() {}
