#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect(const DeviceDescription *device) : Effect(device) {
  if (device->FlagEnabled(Bright)) {
    v = 255;
  } else {
    v = 128;
  }
}

CRGB RainbowEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];
  // Check for whether the entire palette is the same color - if so, change the
  // brightness rather than the hue.
  if (palette.Size() < 2) {
    // Solid color palette
    if (device->FlagEnabled(Tiny)) {
      return palette.GetGradient((cubicwave8(timeMs / 16)) << 8);
    } else {
      CHSV color = palette.GetColor(0);
      if (device->FlagEnabled(Bright)) {
        color.v = cubicwave8(timeMs / 16 + ledIndex * 8);
      } else {
        color.v = (cubicwave8(timeMs / 16 + ledIndex * 8) * (uint16_t)2) / 3;
      }
      return color;
    }
  } else {
    // Varying color palette
    if (device->FlagEnabled(Tiny)) {
      CHSV color = palette.GetGradient((timeMs / 16) << 8);
      color.v = v;
      return color;
    } else {
      CHSV color = palette.GetGradient((timeMs / 16 + ledIndex * 8) << 8);
      color.v = v;
      return color;
    }
  }
}
