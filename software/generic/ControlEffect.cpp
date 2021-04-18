#include "ControlEffect.hpp"

static inline uint8_t min(uint8_t a, uint8_t b) {
  if (a < b) {
    return a;
  } else {
    return b;
  }
}

ControlEffect::ControlEffect(uint8_t numLeds, DeviceType deviceType)
    : Effect(numLeds, deviceType) {}

CRGB ControlEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                              RadioPacket *setEffectPacket) {
  uint8_t r = setEffectPacket->readRFromSetRgb();
  uint8_t g = setEffectPacket->readGFromSetRgb();
  uint8_t b = setEffectPacket->readBFromSetRgb();

  return CRGB(r, g ,b);
}
