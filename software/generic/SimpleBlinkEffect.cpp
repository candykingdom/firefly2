#include "SimpleBlinkEffect.hpp"

SimpleBlinkEffect::SimpleBlinkEffect(const DeviceDescription *device,
                                     uint16_t speed)
    : Effect(device), speed(speed) {}

CRGB SimpleBlinkEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                               RadioPacket *setEffectPacket) {
  const uint32_t chunk = (time_ms / (speed)) % 3;
  if (chunk == 0) {
    ColorPalette palette =
        palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
    CHSV color = palette.GetGradient((time_ms / 4) * 23);
    if (!device->FlagEnabled(Bright)) {
      color.v /= 2;
    }
    return color;
  } else {
    return {0, 0, 0};
  }
}
