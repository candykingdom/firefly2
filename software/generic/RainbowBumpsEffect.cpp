#include "RainbowBumpsEffect.hpp"

RainbowBumpsEffect::RainbowBumpsEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB RainbowBumpsEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                                RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];

  CHSV color = palette.GetGradient((timeMs / 10 + ledIndex * 8) << 8);
  color.v = GetThresholdSin(-(timeMs / 16 - ledIndex * 24) << 8, 0);
  return color;
}
