#include "RainbowBumpsEffect.hpp"

#include <Math.hpp>

RainbowBumpsEffect::RainbowBumpsEffect() : Effect() {}

CRGB RainbowBumpsEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                const StripDescription *strip,
                                RadioPacket *setEffectPacket) {
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[palette_index];

  uint8_t led_count = strip->led_count;
  if (strip->FlagEnabled(Mirrored)) {
    MirrorIndex(&led_index, &led_count);
  }

  uint8_t offset;
  if (strip->FlagEnabled(Circular)) {
    offset = (uint16_t)led_index * 255 / led_count;
  } else {
    offset = led_index * 8;
  }

  CHSV color = palette.GetGradient((time_ms / 10 + offset) << 8);
  color.v = GetThresholdSin(-(time_ms / 16 - offset * 3) << 8, 0);
  return color;
}
