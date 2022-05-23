#include "DisplayColorPaletteEffect.hpp"

DisplayColorPaletteEffect::DisplayColorPaletteEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB DisplayColorPaletteEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                                       RadioPacket *setEffectPacket) {
  ColorPalette palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color = palette.GetGradient((timeMs / 2) * 23);
  if (device->type == DeviceType::Wearable) {
    color.v /= 2;
  }
  return color;
}
