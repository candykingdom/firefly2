#include "Effect.hpp"

Effect::Effect(uint8_t numLeds) : numLeds(numLeds) {}

CRGB Effect::GetRGB(uint8_t ledIndex, uint32_t timeMs) {
  return CRGB(255, 0, 0);
}
