#include "SimpleBlinkEffect.hpp"

SimpleBlinkEffect::SimpleBlinkEffect(uint16_t speed) : Effect(), speed(speed) {}

CRGB SimpleBlinkEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                               const StripDescription *strip,
                               RadioPacket *setEffectPacket) {
  UNUSED(led_index);
  const uint32_t chunk = (time_ms / (speed)) % 3;
  if (chunk == 0) {
    ColorPalette palette =
        palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
    CHSV color = palette.GetGradient((time_ms / 4) * 23);
    if (!strip->FlagEnabled(Bright)) {
      color.v /= 2;
    }
    return color;
  } else {
    return {0, 0, 0};
  }
}
