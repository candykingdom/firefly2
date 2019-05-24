#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect(uint8_t numLeds, CRGB &color)
    : Effect(numLeds), color(color) {}

CRGB RainbowEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];
  // Check for whether the entire palette is the same color - if so, change the
  // brightness rather than the hue.
  if (palette.Size() < 2) {
    // Solid color palette
    if (numLeds < 8) {
      return palette.GetGradient((cubicwave8(timeMs / 16)) << 8);
    } else {
      CHSV color = palette.GetColor(0);
      color.v = (cubicwave8(timeMs / 16 + ledIndex * 8) * (uint16_t)2) / 3;
      return color;
    }
  } else {
    // Varying color palette
    if (numLeds < 8) {
      CHSV color = palette.GetGradient((timeMs / 16) << 8);
      color.v = 128;
      return color;
    } else {
      CHSV color = palette.GetGradient((timeMs / 16 + ledIndex * 8) << 8);
      color.v = 128;
      return color;
    }
  }
}
