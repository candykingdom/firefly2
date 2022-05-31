#include "SparkEffect.hpp"

#include <Math.hpp>

SparkEffect::SparkEffect(const DeviceDescription *device) : Effect(device) {}

CRGB SparkEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                         RadioPacket *setEffectPacket) {
  const uint8_t pulse_size = brightnesses.size();
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[palette_index];

  uint8_t led_count = device->led_count;
  if (device->FlagEnabled(Mirrored)) {
    MirrorIndex(&led_index, &led_count);
  }

  int16_t relative_pos;
  bool reverse = false;
  if (device->FlagEnabled(Circular)) {
    relative_pos = (time_ms * led_count / 3000 + led_index) % led_count;
  } else {
    int16_t pos = ((time_ms * (led_count + pulse_size)) / 3000) %
                  ((led_count + pulse_size) * 2);

    if (pos > (led_count + pulse_size)) {
      reverse = true;
      pos = (led_count + pulse_size) - (pos - (led_count + pulse_size));
    }
    relative_pos = pos - led_index;
  }
  CHSV hsv = palette.GetGradient((time_ms / 24 + relative_pos * 14) << 8);

  if (relative_pos < 0) {
    hsv.v = 0;
  } else if (relative_pos < (led_count + pulse_size)) {
    if (relative_pos < pulse_size) {
      if (reverse) {
        hsv.v = brightnesses[pulse_size - relative_pos - 1];
      } else {
        hsv.v = brightnesses[relative_pos];
      }
    } else {
      hsv.v = 0;
    }
  } else {
    hsv.v = 0;
  }

  return hsv;
}
