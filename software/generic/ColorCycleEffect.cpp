#include "ColorCycleEffect.hpp"
#include "ColorPalette.hpp"

ColorCycleEffect::ColorCycleEffect(uint8_t numLeds, CRGB &color)
    : Effect(numLeds), color(color) {}

CRGB ColorCycleEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                              RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  const ColorPalette palette = palettes[paletteIndex];
  // Check for whether the entire palette is the same color - if so, change the
  // brightness rather than the hue.
  if (palette.Size() < 2) {
    // Solid color palette
    CHSV color = palette.GetColor(0);
    color.v = ((uint16_t)cubicwave8(timeMs / 16)) * 2 / 3;
    return color;
  } else {
    CHSV color = palette.GetGradient((timeMs / 16) << 8);
    color.v /= 2;
    return color;
  }
}
