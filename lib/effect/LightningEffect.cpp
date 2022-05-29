#include "LightningEffect.hpp"

#include "../math/Perlin.hpp"

LightningEffect::LightningEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB LightningEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                             RadioPacket *setEffectPacket) {
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[palette_index];

  uint8_t noise = perlinNoise(led_index << 6, time_ms / 6);
  CHSV color = palette.GetGradient(((noise - 160) << 9) + time_ms);
  if (noise > 160) {
    color.v = noise;
  } else {
    color.v = 0;
  }
  return color;
}
