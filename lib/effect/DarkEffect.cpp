#include "DarkEffect.hpp"

DarkEffect::DarkEffect() : Effect() {}

CRGB DarkEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                        const StripDescription *strip,
                        RadioPacket *setEffectPacket) {
  return {0, 0, 0};
}
