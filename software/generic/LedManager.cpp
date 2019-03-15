#include "LedManager.hpp"

LedManager::LedManager(const uint8_t numLeds) : numLeds(numLeds) {}

void LedManager::RunEffect(uint32_t timeMillis) {
  for (uint8_t ledIndex = 0; ledIndex < numLeds; ledIndex++) {
    CRGB rgb = effects[effectIndex].GetRGB(ledIndex, timeMillis);
    SetLed(ledIndex, rgb);
  }
}

void LedManager::SetEffect(uint8_t effectIndex) {
  this->effectIndex = effectIndex;
}
