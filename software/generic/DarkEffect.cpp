#include "DarkEffect.hpp"

DarkEffect::DarkEffect(uint8_t numLeds) : Effect(numLeds) {}

CRGB DarkEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                        RadioPacket *setEffectPacket) {
  return {0, 0, 0};
}
