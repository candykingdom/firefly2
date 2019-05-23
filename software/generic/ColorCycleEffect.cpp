#include "ColorCycleEffect.hpp"

ColorCycleEffect::ColorCycleEffect(uint8_t numLeds, CRGB &color)
    : Effect(numLeds), color(color) {}

CRGB ColorCycleEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                              RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  // Rough check for whether the entire palette is the same color - if so,
  // change the brightness rather than the hue.
  if (ColorFromPalette(palettes[paletteIndex], 0) ==
      ColorFromPalette(palettes[paletteIndex], 64)) {
    // Solid color palette
    return ColorFromPalette(palettes[paletteIndex], 0,
                            ((uint16_t)cubicwave8(timeMs / 16)) * 2 / 3);
  } else {
    return ColorFromPalette(palettes[paletteIndex], timeMs / 16, 128);
  }
}
