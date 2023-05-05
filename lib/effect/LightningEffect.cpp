#include "LightningEffect.hpp"

#include <Perlin.hpp>

LightningEffect::LightningEffect() : Effect() {}

CRGB LightningEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                             const StripDescription &strip,
                             RadioPacket *setEffectPacket) const {
  UNUSED(led_index);
  UNUSED(strip);
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes()[palette_index];

  uint8_t noise = perlinNoise(led_index << 6, time_ms / 6);
  CHSV color = palette.GetGradient(((noise - 160) * 512) + time_ms);
  if (noise > 160) {
    color.v = noise;
  } else {
    color.v = 0;
  }
  return color;
}
