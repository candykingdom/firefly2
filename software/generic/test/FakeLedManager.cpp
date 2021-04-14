#include "FakeLedManager.hpp"

#include <cassert>

FakeLedManager::FakeLedManager(const uint8_t numLeds,
                               RadioStateMachine *stateMachine)
    : LedManager(numLeds, DeviceType::Wearable, stateMachine) {
  leds = new CRGB[numLeds];
}

void FakeLedManager::SetGlobalColor(CRGB rgb) {
  for (uint8_t i = 0; i < numLeds; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint8_t ledIndex) {
  assert(ledIndex < numLeds);
  return leds[ledIndex];
}

void FakeLedManager::ClearEffects() {
  effects.clear();
  nonRandomEffects.clear();
  uniqueEffectIndices.clear();
}

void FakeLedManager::PublicAddEffect(Effect *effect, uint8_t proportion) {
  AddEffect(effect, proportion);
}

void FakeLedManager::SetLed(uint8_t ledIndex, CRGB *const rgb) {
  assert(ledIndex < numLeds);
  leds[ledIndex] = *rgb;
}

void FakeLedManager::WriteOutLeds() {}
