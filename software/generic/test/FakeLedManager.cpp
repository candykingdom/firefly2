#include "FakeLedManager.hpp"
#include <cassert>
#include "FireEffect.hpp"
#include "PoliceEffect.hpp"
#include "SimpleBlinkEffect.hpp"

FakeLedManager::FakeLedManager(const uint8_t numLeds,
                               RadioStateMachine *stateMachine)
    : LedManager(numLeds, stateMachine) {
  leds = new CRGB[numLeds];

  // Set the effects for test
  effects.clear();
  nonRandomEffects.clear();
  uniqueEffectIndices.clear();

  AddEffect(new SimpleBlinkEffect(numLeds), 4);
  AddEffect(new PoliceEffect(numLeds), 0);
  AddEffect(new FireEffect(numLeds), 2);
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

void FakeLedManager::SetLed(uint8_t ledIndex, CRGB *const rgb) {
  assert(ledIndex < numLeds);
  leds[ledIndex] = *rgb;
}

void FakeLedManager::WriteOutLeds() {}
