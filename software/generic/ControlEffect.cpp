#include "ControlEffect.hpp"

ControlEffect::ControlEffect(uint8_t numLeds, DeviceType deviceType)
    : Effect(numLeds, deviceType) {}

CRGB ControlEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  return setEffectPacket->readRgbFromSetControl();
}
