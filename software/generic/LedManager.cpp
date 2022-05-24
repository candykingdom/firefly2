#include "LedManager.hpp"

#include <cassert>

#include "ColorCycleEffect.hpp"
#include "ContrastBumpsEffect.hpp"
#include "DarkEffect.hpp"
#include "DisplayColorPaletteEffect.hpp"
#include "FireEffect.hpp"
#include "FireflyEffect.hpp"
#include "LightningEffect.hpp"
#include "PoliceEffect.hpp"
#include "RainbowBumpsEffect.hpp"
#include "RainbowEffect.hpp"
#include "RorschachEffect.hpp"
#include "SimpleBlinkEffect.hpp"
#include "SparkEffect.hpp"
#include "StopLightEffect.hpp"
#include "SwingingLights.hpp"

LedManager::LedManager(const DeviceDescription *device,
                       RadioStateMachine *radioState)
    : device(device), radioState(radioState) {
  AddEffect(new ColorCycleEffect(device), 4);
  AddEffect(new ContrastBumpsEffect(device), 2);
  AddEffect(new FireEffect(device), 1);
  AddEffect(new FireflyEffect(device), 2);
  AddEffect(new LightningEffect(device), 1);
  AddEffect(new RainbowBumpsEffect(device), 4);
  AddEffect(new RainbowEffect(device), 4);
  AddEffect(new RorschachEffect(device), 2);
  AddEffect(new SimpleBlinkEffect(device, 300), 2);
  AddEffect(new SparkEffect(device), 4);
  AddEffect(new SwingingLights(device), 4);

  // Non-random effects
  AddEffect(new PoliceEffect(device), 0);
  AddEffect(new StopLightEffect(device), 0);
  // Strobes
  AddEffect(new SimpleBlinkEffect(device, 60), 0);
  AddEffect(new SimpleBlinkEffect(device, 30), 0);
  AddEffect(new SimpleBlinkEffect(device, 12), 0);

  // These two must be last
  AddEffect(new DisplayColorPaletteEffect(device), 0);
  AddEffect(new DarkEffect(device), 0);

  controlEffect = new ControlEffect(device);

  radioState->SetNumEffects(GetNumEffects());
  radioState->SetNumPalettes(effects[0]->palettes.size());
}

Effect *LedManager::GetCurrentEffect() {
  RadioPacket *packet = radioState->GetSetEffect();
  if (packet->type == SET_CONTROL) {
    return controlEffect;
  }
  uint8_t effectIndex =
      radioState->GetSetEffect()->readEffectIndexFromSetEffect();
  return GetEffect(effectIndex);
}

Effect *LedManager::GetEffect(uint8_t index) {
  uint8_t totalNumEffects = effects.size() + nonRandomEffects.size();
#ifdef ARDUINO
  index = index % totalNumEffects;
#else
  assert(index < totalNumEffects);
#endif

  if (index < effects.size()) {
    return effects[index];
  } else {
    return nonRandomEffects[index - effects.size()];
  }
}

void LedManager::RunEffect() {
  for (uint8_t ledIndex = 0; ledIndex < device->physical_leds; ledIndex++) {
    CRGB rgb = GetCurrentEffect()->GetRGB(device->PhysicalToVirtual(ledIndex),
                                          radioState->GetNetworkMillis(),
                                          radioState->GetSetEffect());
    SetLed(ledIndex, &rgb);
  }
  WriteOutLeds();
}

uint8_t LedManager::GetNumEffects() { return effects.size(); }

uint8_t LedManager::GetNumUniqueEffects() {
  return uniqueEffectIndices.size() + nonRandomEffects.size();
}

uint8_t LedManager::GetNumNonRandomEffects() { return nonRandomEffects.size(); }

uint8_t LedManager::UniqueEffectNumberToIndex(uint8_t uniqueEffectNumber) {
  if (uniqueEffectNumber < uniqueEffectIndices.size()) {
    return uniqueEffectIndices[uniqueEffectNumber];
  } else {
    return effects.size() + (uniqueEffectNumber - uniqueEffectIndices.size());
  }
}

void LedManager::AddEffect(Effect *effect, uint8_t proportion) {
  if (proportion > 0) {
    uniqueEffectIndices.push_back(effects.size());
  } else {
    nonRandomEffects.push_back(effect);
  }
  for (uint8_t i = 0; i < proportion; ++i) {
    effects.push_back(effect);
  }
#ifndef ARDUINO
  // The effect index is 8 bits, so make sure the total number of effects is in
  // range.
  assert(effects.size() + nonRandomEffects.size() < 256);
#endif
}
