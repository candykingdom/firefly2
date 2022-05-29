#include "ContrastBumpsEffect.hpp"

#include "../math/Math.hpp"

ContrastBumpsEffect::ContrastBumpsEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB ContrastBumpsEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
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
    offset = led_index * 24;
  }

  uint8_t sin = GetThresholdSin(-(time_ms / 16 + offset) << 8, 0);
  uint16_t color_index = time_ms * 16;
  if (sin == 0) {
    CHSV color = palette.GetGradient(color_index);
    color.v = 64;
    return color;
  } else {
    CHSV color = palette.GetGradient(color_index + 0x4000);
    color.v = qadd8(sin / 2, 96);
    return color;
  }
}
