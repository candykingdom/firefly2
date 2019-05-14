#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect(uint8_t numLeds, CRGB &color)
    : Effect(numLeds), color(color) {}

CRGB RainbowEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  // Rough check for whether the entire palette is the same color - if so,
  // change the brightness rather than the hue.
  if (ColorFromPalette(palettes[paletteIndex], 0) ==
      ColorFromPalette(palettes[paletteIndex], 64)) {
    // Solid color palette
    if (numLeds < 8) {
      return ColorFromPalette(palettes[paletteIndex], 0,
                              cubicwave8(timeMs / 16));
    } else {
      return ColorFromPalette(
          palettes[paletteIndex], 0,
          (cubicwave8(timeMs / 16 + ledIndex * 8) * (uint16_t)2) / 3);
    }
  } else {
    // Varying color palette
    if (numLeds < 8) {
      return ColorFromPalette(palettes[paletteIndex], timeMs / 16, 128);
    } else {
      return ColorFromPalette(palettes[paletteIndex],
                              timeMs / 16 + ledIndex * 8, 128);
    }
  }
}
