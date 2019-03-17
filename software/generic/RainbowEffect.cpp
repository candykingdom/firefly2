#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect(uint8_t numLeds, CRGB &color)
    : Effect(numLeds), color(color) {}

CRGB RainbowEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs) {
  if (numLeds < 8) {
    return CHSV(timeMs / 16, 255, 128);
  } else {
    return CHSV(timeMs / 16 + ledIndex * 8, 255, 128);
  }
}
