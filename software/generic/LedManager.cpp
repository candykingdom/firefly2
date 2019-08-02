#include "LedManager.hpp"
#include <cassert>
#include "ColorCycleEffect.hpp"
#include "FireEffect.hpp"
#include "FireflyEffect.hpp"
#include "PoliceEffect.hpp"
#include "RainbowEffect.hpp"
#include "RorschachEffect.hpp"
#include "SimpleBlinkEffect.hpp"
#include "StopLightEffect.hpp"
#include "SwingingLights.hpp"

LedManager::LedManager(const uint8_t numLeds, RadioStateMachine *radioState)
    : numLeds(numLeds), radioState(radioState) {
  CRGB color = CRGB::Red;
  AddEffect(new ColorCycleEffect(numLeds, color), 16);
  AddEffect(new FireEffect(numLeds), 4);
  AddEffect(new FireflyEffect(numLeds, color), 16);
  AddEffect(new PoliceEffect(numLeds), 0);
  AddEffect(new RainbowEffect(numLeds, color), 16);
  AddEffect(new RorschachEffect(numLeds), 16);
  AddEffect(new SimpleBlinkEffect(numLeds), 16);
  AddEffect(new StopLightEffect(numLeds), 4);
  AddEffect(new SwingingLights(numLeds), 16);
  radioState->SetNumEffects(GetNumEffects());
  radioState->SetNumPalettes(effects[0]->palettes.size());
}

Effect *LedManager::GetCurrentEffect() {
  uint8_t effectIndex =
      radioState->GetSetEffect()->readEffectIndexFromSetEffect();
#ifdef ARDUINO
  effectIndex = effectIndex % effects.size();
#else
  assert(effectIndex < GetNumEffects());
#endif
  return effects[effectIndex];
}

Effect *LedManager::GetEffect(uint8_t index) {
#ifdef ARDUINO
  index = index % effects.size();
#else
  assert(index < effects.size());
#endif
  return effects[index];
}

void LedManager::RunEffect() {
  for (uint8_t ledIndex = 0; ledIndex < numLeds; ledIndex++) {
    CRGB rgb = GetCurrentEffect()->GetRGB(
        ledIndex, radioState->GetNetworkMillis(), radioState->GetSetEffect());
    SetLed(ledIndex, &rgb);
  }
  WriteOutLeds();
}

uint8_t LedManager::GetNumEffects() { return effects.size(); }

uint8_t LedManager::GetNumUniqueEffects() { return uniqueEffectIndices.size(); }

uint8_t LedManager::UniqueEffectNumberToIndex(uint8_t uniqueEffectNumber) {
  return uniqueEffectIndices[uniqueEffectNumber];
}

void LedManager::AddEffect(Effect *effect, uint8_t proportion) {
  if (proportion > 0) {
    uniqueEffectIndices.push_back(effects.size());
  }
  for (uint8_t i = 0; i < proportion; ++i) {
    effects.push_back(effect);
  }
}
