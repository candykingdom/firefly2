#include "FakeLedManager.hpp"

#include <cassert>

FakeLedManager::FakeLedManager(const DeviceDescription &device,
                               RadioStateMachine *state_machine)
    : LedManager(device, state_machine) {
  led_count = device.GetLedCount();

  leds = new CRGB[led_count];
}

FakeLedManager::~FakeLedManager() { delete[] leds; }

void FakeLedManager::SetGlobalColor(const CRGB &rgb) {
  for (uint8_t i = 0; i < led_count; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint16_t led_index) {
#ifndef ARDUINO
  assert(led_index < led_count);
#endif
  return leds[led_index];
}

void FakeLedManager::ClearEffects() {
  for (uint8_t effect_index : uniqueEffectIndices) {
    delete effects[effect_index];
  }
  for (Effect *effect : non_random_effects) {
    delete effect;
  }
  effects.clear();
  non_random_effects.clear();
  uniqueEffectIndices.clear();
}

void FakeLedManager::PublicAddEffect(Effect *effect, uint8_t proportion) {
  AddEffect(effect, proportion);
}

void FakeLedManager::SetLed(uint16_t led_index, const CRGB &rgb) {
#ifndef ARDUINO
  assert(led_index < led_count);
#endif
  leds[led_index] = rgb;
}

void FakeLedManager::WriteOutLeds() {}
