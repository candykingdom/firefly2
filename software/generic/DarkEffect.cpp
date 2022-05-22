#include "DarkEffect.hpp"

DarkEffect::DarkEffect(DeviceDescription *const device) : Effect(device) {}

CRGB DarkEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                        RadioPacket *setEffectPacket) {
  return {0, 0, 0};
}
