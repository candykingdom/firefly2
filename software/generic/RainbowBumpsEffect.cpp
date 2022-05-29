#include "RainbowBumpsEffect.hpp"

#include "Math.hpp"

RainbowBumpsEffect::RainbowBumpsEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB RainbowBumpsEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                                RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];

  uint8_t led_count = device->led_count;
  if (device->FlagEnabled(Mirrored)) {
    MirrorIndex(&ledIndex, &led_count);
  }

  uint8_t offset;
  if (device->FlagEnabled(Circular)) {
    offset = (uint16_t)ledIndex * 255 / led_count;
  } else {
    offset = ledIndex * 8;
  }

  CHSV color = palette.GetGradient((timeMs / 10 + offset) << 8);
  color.v = GetThresholdSin(-(timeMs / 16 - offset * 3) << 8, 0);
  return color;
}
