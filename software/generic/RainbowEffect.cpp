#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect(const DeviceDescription *device)
    : Effect(device) {
  switch (device->type) {
    // Wearable gets reduced brightness, since we're lighting the whole strip.
    case DeviceType::Wearable:
      v = 128;
      break;
    case DeviceType::Bike:
      v = 255;
      break;
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
    if (device->virtual_leds < 8) {
      return palette.GetGradient((cubicwave8(timeMs / 16)) << 8);
    } else {
      CHSV color = palette.GetColor(0);
      switch (device->type) {
        case DeviceType::Wearable:
          color.v = (cubicwave8(timeMs / 16 + ledIndex * 8) * (uint16_t)2) / 3;
          break;
        case DeviceType::Bike:
          color.v = cubicwave8(timeMs / 16 + ledIndex * 8);
          break;
      }
      return color;
    }
  } else {
    // Varying color palette
    if (device->virtual_leds < 8) {
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
