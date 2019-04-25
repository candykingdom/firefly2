#include "LedManager.hpp"
#include "RainbowEffect.hpp"

LedManager::LedManager(const uint8_t numLeds) : numLeds(numLeds) {
  CRGB color = CRGB::Red;
  effects.push_back(new RainbowEffect(numLeds, color));
}

Effect *LedManager::GetCurrentEffect() { return effects[effectIndex]; }

void LedManager::RunEffect(uint32_t timeMillis, RadioPacket *setEffectPacket) {
  for (uint8_t ledIndex = 0; ledIndex < numLeds; ledIndex++) {
    CRGB rgb =
        effects[effectIndex]->GetRGB(ledIndex, timeMillis, setEffectPacket);
    SetLed(ledIndex, &rgb);
  }
  WriteOutLeds();
}

void LedManager::SetEffect(uint8_t effectIndex) {
  // TODO: add a (#define-guarded?) check that effectIndex is in range?
  this->effectIndex = effectIndex % effects.size();
}

uint8_t LedManager::GetNumEffects() { return effects.size(); }
