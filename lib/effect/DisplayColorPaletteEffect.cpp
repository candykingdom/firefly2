#include "DisplayColorPaletteEffect.hpp"

DisplayColorPaletteEffect::DisplayColorPaletteEffect(
    const DeviceDescription *device)
    : Effect(device) {}

CRGB DisplayColorPaletteEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                       RadioPacket *setEffectPacket) {
  ColorPalette palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color = palette.GetGradient((time_ms / 2) * 23);
  if (!device->FlagEnabled(Bright)) {
    color.v /= 2;
  }
  return color;
}
