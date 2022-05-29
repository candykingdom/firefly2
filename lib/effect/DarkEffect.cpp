#include "DarkEffect.hpp"

DarkEffect::DarkEffect(const DeviceDescription *device) : Effect(device) {}

CRGB DarkEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                        RadioPacket *setEffectPacket) {
  return {0, 0, 0};
}
