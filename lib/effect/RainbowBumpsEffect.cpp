#include "RainbowBumpsEffect.hpp"

#include "../math/Math.hpp"

RainbowBumpsEffect::RainbowBumpsEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB RainbowBumpsEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                RadioPacket *setEffectPacket) {
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[palette_index];

  uint8_t led_count = device->led_count;
  if (device->FlagEnabled(Mirrored)) {
    MirrorIndex(&led_index, &led_count);
  }

  uint8_t offset;
  if (device->FlagEnabled(Circular)) {
    offset = (uint16_t)led_index * 255 / led_count;
  } else {
    offset = led_index * 8;
  }

  CHSV color = palette.GetGradient((time_ms / 10 + offset) << 8);
  color.v = GetThresholdSin(-(time_ms / 16 - offset * 3) << 8, 0);
  return color;
}
