#include "FakeLedManager.hpp"

#include <cassert>

FakeLedManager::FakeLedManager(const DeviceDescription *device,
                               RadioStateMachine *stateMachine)
    : LedManager(device, stateMachine) {
  leds = new CRGB[device->led_count];
}

void FakeLedManager::SetGlobalColor(CRGB rgb) {
  for (uint8_t i = 0; i < device->led_count; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint8_t ledIndex) {
  assert(ledIndex < device->led_count);
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
  assert(ledIndex < device->led_count);
  leds[ledIndex] = *rgb;
}

void FakeLedManager::WriteOutLeds() {}
