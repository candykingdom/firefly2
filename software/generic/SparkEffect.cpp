#include "SparkEffect.hpp"

SparkEffect::SparkEffect(uint8_t numLeds) : Effect(numLeds) {}

CRGB SparkEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                         RadioPacket *setEffectPacket) {
  const uint8_t pulse_size = brightnesses.size();
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  ColorPalette palette = palettes[paletteIndex];
  int16_t pos = ((timeMs * (numLeds + pulse_size)) / 3000) % ((numLeds + pulse_size) * 2);

  bool reverse = false;
  if (pos > (numLeds + pulse_size)) {
    reverse = true;
    pos = (numLeds + pulse_size) - (pos - (numLeds + pulse_size));
  }
  int16_t relative_pos = pos - ledIndex;
  CHSV hsv = palette.GetGradient((timeMs / 24 + relative_pos * 14) << 8);

  if (relative_pos < 0) {
    hsv.v = 0;
  } else if (relative_pos < (numLeds + pulse_size)) {
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
