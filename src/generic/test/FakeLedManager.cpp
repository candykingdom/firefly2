#include "FakeLedManager.hpp"

#include <cassert>

FakeLedManager::FakeLedManager(const DeviceDescription *device,
                               RadioStateMachine *state_machine)
    : LedManager(device, state_machine) {
  led_count = 0;
  for (auto it = device->strips.begin(); it != device->strips.end(); ++it) {
    led_count += (*it)->led_count;
  }

  leds = new CRGB[led_count];
}

FakeLedManager::~FakeLedManager() { delete[] leds; }

void FakeLedManager::SetGlobalColor(const CRGB &rgb) {
  for (uint8_t i = 0; i < led_count; i++) {
    leds[i] = rgb;
  }
}

CRGB FakeLedManager::GetLed(uint8_t led_index) {
  assert(led_index < led_count);
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

void FakeLedManager::SetLed(uint8_t led_index, const CRGB &rgb) {
  assert(led_index < led_count);
  leds[led_index] = rgb;
}

void FakeLedManager::WriteOutLeds() {}
