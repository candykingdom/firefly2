#include "LedManager.hpp"
#include <cassert>
#include "ColorCycleEffect.hpp"
#include "DarkEffect.hpp"
#include "DisplayColorPaletteEffect.hpp"
#include "FireEffect.hpp"
#include "FireflyEffect.hpp"
#include "PoliceEffect.hpp"
#include "RainbowBumpsEffect.hpp"
#include "RainbowEffect.hpp"
#include "RorschachEffect.hpp"
#include "SimpleBlinkEffect.hpp"
#include "StopLightEffect.hpp"
#include "SwingingLights.hpp"

LedManager::LedManager(const uint8_t numLeds, RadioStateMachine *radioState)
    : numLeds(numLeds), radioState(radioState) {
  CRGB color = CRGB::Red;
  AddEffect(new ColorCycleEffect(numLeds), 16);
  AddEffect(new FireEffect(numLeds), 4);
  AddEffect(new FireflyEffect(numLeds, color), 16);
  AddEffect(new RainbowBumpsEffect(numLeds, color), 16);
  AddEffect(new RainbowEffect(numLeds, color), 16);
  AddEffect(new RorschachEffect(numLeds), 16);
  AddEffect(new SimpleBlinkEffect(numLeds), 16);
  AddEffect(new SwingingLights(numLeds), 16);

  // Non-random effects
  AddEffect(new PoliceEffect(numLeds), 0);
  AddEffect(new StopLightEffect(numLeds), 0);

  // These two must be last
  AddEffect(new DisplayColorPaletteEffect(numLeds), 0);
  AddEffect(new DarkEffect(numLeds), 0);

  radioState->SetNumEffects(GetNumEffects());
  radioState->SetNumPalettes(effects[0]->palettes.size());
}

Effect *LedManager::GetCurrentEffect() {
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
  for (uint8_t ledIndex = 0; ledIndex < numLeds; ledIndex++) {
    CRGB rgb = GetCurrentEffect()->GetRGB(
        ledIndex, radioState->GetNetworkMillis(), radioState->GetSetEffect());
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
}
