#include "SimpleBlinkEffect.hpp"

SimpleBlinkEffect::SimpleBlinkEffect(uint8_t numLeds) : Effect(numLeds) {}

CRGB SimpleBlinkEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                               RadioPacket *setEffectPacket) {
  // 1s period, divided into 100ms chunks
  const uint32_t chunk = (timeMs / 100) % 10;
  if (chunk < 3) {
    ColorPalette palette =
        palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
    CHSV color = palette.GetGradient((timeMs / 4) * 23);
    color.v /= 2;
    return color;
  } else {
    return {0, 0, 0};
  }
}
