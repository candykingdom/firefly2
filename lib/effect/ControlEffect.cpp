#include "ControlEffect.hpp"

ControlEffect::ControlEffect() : Effect() {}

CRGB ControlEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                           const StripDescription *strip,
                           RadioPacket *setEffectPacket) {
  return setEffectPacket->readRgbFromSetControl();
}
