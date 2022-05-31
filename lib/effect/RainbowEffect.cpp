#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect() : Effect() {}

CRGB RainbowEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                           const StripDescription *strip,
                           RadioPacket *setEffectPacket) {
  uint8_t v;
  if (strip->FlagEnabled(Bright)) {
    v = 255;
  } else {
    v = 128;
  }

  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[palette_index];
  // Check for whether the entire palette is the same color - if so, change the
  // brightness rather than the hue.
  if (palette.Size() < 2) {
    // Solid color palette
    if (strip->FlagEnabled(Tiny)) {
      return palette.GetGradient((cubicwave8(time_ms / 16)) << 8);
    } else {
      CHSV color = palette.GetColor(0);
      if (strip->FlagEnabled(Bright)) {
        color.v = cubicwave8(time_ms / 16 + led_index * 8);
      } else {
        color.v = (cubicwave8(time_ms / 16 + led_index * 8) * (uint16_t)2) / 3;
      }
      return color;
    }
  } else {
    // Varying color palette
    if (strip->FlagEnabled(Tiny)) {
      CHSV color = palette.GetGradient((time_ms / 16) << 8);
      color.v = v;
      return color;
    } else {
      CHSV color = palette.GetGradient((time_ms / 16 + led_index * 8) << 8);
      color.v = v;
      return color;
    }
  }
}
