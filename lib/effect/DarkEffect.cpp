#include "DarkEffect.hpp"

DarkEffect::DarkEffect() : Effect() {}

CRGB DarkEffect::GetRGB(uint16_t led_index, uint32_t time_ms,
                        const StripDescription &strip,
                        RadioPacket *setEffectPacket) const {
  UNUSED(led_index);
  UNUSED(time_ms);
  UNUSED(strip);
  UNUSED(setEffectPacket);
  return {0, 0, 0};
}
