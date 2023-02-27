#include "ControlEffect.hpp"

ControlEffect::ControlEffect() : Effect() {}

CRGB ControlEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                           const StripDescription &strip,
                           RadioPacket *setEffectPacket) {
  UNUSED(led_index);
  UNUSED(time_ms);
  UNUSED(strip);
  UNUSED(setEffectPacket);
  return setEffectPacket->readRgbFromSetControl();
}
