#include "DisplayColorPaletteEffect.hpp"

DisplayColorPaletteEffect::DisplayColorPaletteEffect(uint8_t numLeds,
                                                     DeviceType deviceType)
    : Effect(numLeds, deviceType) {}

CRGB DisplayColorPaletteEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                                       RadioPacket *setEffectPacket) {
  ColorPalette palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color = palette.GetGradient((timeMs / 2) * 23);
  if (deviceType == DeviceType::Wearable) {
    color.v /= 2;
  }
  return color;
}
