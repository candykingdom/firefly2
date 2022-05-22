#include "ControlEffect.hpp"

ControlEffect::ControlEffect(DeviceDescription *const device)
    : Effect(device) {}

CRGB ControlEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  return setEffectPacket->readRgbFromSetControl();
}
