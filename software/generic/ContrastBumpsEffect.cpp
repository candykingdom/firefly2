#include "ContrastBumpsEffect.hpp"

ContrastBumpsEffect::ContrastBumpsEffect(DeviceDescription *const device) : Effect(device) {}

CRGB ContrastBumpsEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                                 RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];

  uint8_t sin = GetThresholdSin(-(timeMs / 16 + ledIndex * 24) << 8, 0);
  uint16_t colorIndex = timeMs * 16;
  if (sin == 0) {
    CHSV color = palette.GetGradient(colorIndex);
    color.v = 64;
    return color;
  } else {
    CHSV color = palette.GetGradient(colorIndex + 0x4000);
    color.v = qadd8(sin / 2, 96);
    return color;
  }
}
