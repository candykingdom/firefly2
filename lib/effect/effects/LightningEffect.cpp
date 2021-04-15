#include "LightningEffect.hpp"

#include "Perlin.hpp"

LightningEffect::LightningEffect(uint8_t numLeds) : Effect(numLeds) {}

CRGB LightningEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                             RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];

  uint8_t noise = perlinNoise(ledIndex << 6, timeMs / 6);
  CHSV color = palette.GetGradient(((noise - 160) << 9) + timeMs);
  if (noise > 160) {
    color.v = noise;
  } else {
    color.v = 0;
  }
  return color;
}
