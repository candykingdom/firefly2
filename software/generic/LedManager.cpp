#include "LedManager.hpp"
#include <cassert>
#include "ColorCycleEffect.hpp"
#include "FireflyEffect.hpp"
#include "PoliceEffect.hpp"
#include "RainbowEffect.hpp"
#include "SwingingLights.hpp"

LedManager::LedManager(const uint8_t numLeds, RadioStateMachine *radioState)
    : numLeds(numLeds), radioState(radioState) {
  CRGB color = CRGB::Red;
  effects.push_back(new ColorCycleEffect(numLeds, color));
  effects.push_back(new FireflyEffect(numLeds, color));
  effects.push_back(new PoliceEffect(numLeds));
  effects.push_back(new RainbowEffect(numLeds, color));
  effects.push_back(new SwingingLights(numLeds));
  radioState->SetNumEffects(effects.size());
  radioState->SetNumPalettes(effects[0]->palettes.size());
}

Effect *LedManager::GetCurrentEffect() {
  uint8_t effectIndex =
      radioState->GetSetEffect()->readEffectIndexFromSetEffect();
#ifdef ARDUINO
  effectIndex = effectIndex % effects.size();
#else
  assert(effectIndex < effects.size());
#endif
  return effects[effectIndex];
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
