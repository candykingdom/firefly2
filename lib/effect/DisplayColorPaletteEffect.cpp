#include "DisplayColorPaletteEffect.hpp"

DisplayColorPaletteEffect::DisplayColorPaletteEffect() : Effect() {}

CRGB DisplayColorPaletteEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                       const StripDescription *strip,
                                       RadioPacket *setEffectPacket) {
  UNUSED(led_index);
  ColorPalette palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color = palette.GetGradient((time_ms / 2) * 23);
  if (!strip->FlagEnabled(Bright)) {
    color.v /= 2;
  }
  return color;
}
