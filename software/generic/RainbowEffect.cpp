#include "RainbowEffect.hpp"

RainbowEffect::RainbowEffect(uint8_t numLeds, CRGB &color)
    : Effect(numLeds), color(color) {}

CRGB RainbowEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  if (numLeds < 8) {
    return ColorFromPalette(
        palettes[setEffectPacket->readPaletteIndexFromSetEffect()], timeMs / 16,
        128);
  } else {
    return ColorFromPalette(
        palettes[setEffectPacket->readPaletteIndexFromSetEffect()],
        timeMs / 16 + ledIndex * 8, 128);
  }
}
