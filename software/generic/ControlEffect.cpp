#include "ControlEffect.hpp"

ControlEffect::ControlEffect(uint8_t numLeds, DeviceType deviceType)
    : Effect(numLeds, deviceType) {}

CRGB ControlEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                              RadioPacket *setEffectPacket) {
  uint8_t r = setEffectPacket->readRFromSetControl();
  uint8_t g = setEffectPacket->readGFromSetControl();
  uint8_t b = setEffectPacket->readBFromSetControl();

  return CRGB(r, g ,b);
}
