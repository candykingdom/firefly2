#include "SimpleBlinkEffect.hpp"

SimpleBlinkEffect::SimpleBlinkEffect(const DeviceDescription *device,
                                     uint16_t speed)
    : Effect(device), speed(speed) {}

CRGB SimpleBlinkEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                               RadioPacket *setEffectPacket) {
  const uint32_t chunk = (timeMs / (speed)) % 3;
  if (chunk == 0) {
    ColorPalette palette =
        palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
    CHSV color = palette.GetGradient((timeMs / 4) * 23);
    if (!device->FlagEnabled(Bright)) {
      color.v /= 2;
    }
    return color;
  } else {
    return {0, 0, 0};
  }
}
