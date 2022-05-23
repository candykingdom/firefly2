#include "ControlEffect.hpp"

ControlEffect::ControlEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB ControlEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  return setEffectPacket->readRgbFromSetControl();
}
