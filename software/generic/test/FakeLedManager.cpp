#include "FakeLedManager.hpp"

#include <cassert>

FakeLedManager::FakeLedManager(const DeviceDescription *device,
                               RadioStateMachine *state_machine)
    : LedManager(device, state_machine) {
  leds = new CRGB[device->led_count];
}

void FakeLedManager::SetGlobalColor(CRGB rgb) {
  for (uint8_t i = 0; i < device->led_count; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint8_t led_index) {
  assert(led_index < device->led_count);
  return leds[led_index];
}

void FakeLedManager::ClearEffects() {
  effects.clear();
  non_random_effects.clear();
  uniqueEffectIndices.clear();
}

void FakeLedManager::PublicAddEffect(Effect *effect, uint8_t proportion) {
  AddEffect(effect, proportion);
}

void FakeLedManager::SetLed(uint8_t led_index, CRGB *const rgb) {
  assert(led_index < device->led_count);
  leds[led_index] = *rgb;
}

void FakeLedManager::WriteOutLeds() {}
