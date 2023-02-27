#include "DisplayColorPaletteEffect.hpp"

DisplayColorPaletteEffect::DisplayColorPaletteEffect() : Effect() {}

CRGB DisplayColorPaletteEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                       const StripDescription &strip,
                                       RadioPacket *setEffectPacket) {
  ColorPalette palette =
      palettes()[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color;
  if (strip.led_count < palette.Size() && palette.Size() <= 4) {
    color = palette.GetGradient((time_ms / 2) * 23);
  } else {
    color =
        palette.GetGradient((((uint32_t)led_index) * 65536) / strip.led_count);
  }

  if (!strip.FlagEnabled(Bright)) {
    color.v /= 2;
  }
  return color;
}
