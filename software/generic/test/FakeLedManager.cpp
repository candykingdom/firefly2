#include "FakeLedManager.hpp"

#include <cassert>

FakeLedManager::FakeLedManager(DeviceDescription *device,
                               RadioStateMachine *stateMachine)
    : LedManager(device, stateMachine) {
  leds = new CRGB[device->physicalLeds];
}

void FakeLedManager::SetGlobalColor(CRGB rgb) {
  for (uint8_t i = 0; i < device->physicalLeds; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint8_t ledIndex) {
  assert(ledIndex < device->physicalLeds);
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
  assert(ledIndex < device->physicalLeds);
  leds[ledIndex] = *rgb;
}

void FakeLedManager::WriteOutLeds() {}
