#include "ControlEffect.hpp"

ControlEffect::ControlEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB ControlEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                           RadioPacket *setEffectPacket) {
  return setEffectPacket->readRgbFromSetControl();
}
