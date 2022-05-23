#include "FireflyEffect.hpp"

#include <Debug.hpp>

FireflyEffect::FireflyEffect(const DeviceDescription *device) : Effect(device) {
#ifdef ARDUINO
  randomSeed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random(0, kBlinkPeriod / 2);
}

CRGB FireflyEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                           RadioPacket *setEffectPacket) {
  const int8_t phase = (timeMs / kPeriodMs) % 3;

  uint32_t adjustedTime = 0;
  const uint32_t periodStart = (timeMs / kPeriodMs) * kPeriodMs;
  const uint32_t elapsedInPeriod = timeMs - periodStart;
  const uint32_t remainingInPeriod = kPeriodMs - elapsedInPeriod;
  if (phase == 0) {
    // Out of sync -> in sync
    if (offset < kBlinkPeriod / 4) {
      adjustedTime = timeMs - (offset * remainingInPeriod) / kPeriodMs;
    } else {
      adjustedTime = timeMs + (offset * remainingInPeriod) / kPeriodMs;
    }
  } else if (phase == 1) {
    // In sync
    adjustedTime = timeMs;
  } else {
    // In sync -> out of sync
    if (offset < kBlinkPeriod / 4) {
      adjustedTime = timeMs - (offset * elapsedInPeriod) / kPeriodMs;
    } else {
      adjustedTime = timeMs + (offset * elapsedInPeriod) / kPeriodMs;
    }
  }

  int16_t curve = sin16(adjustedTime * kSinMultiplier);
  if (curve < 0) {
    curve = 0;
  }
  ColorPalette palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color = palette.GetGradient((timeMs / kBlinkPeriod) << 8);
  color.v = curve / 256;
  return color;
}
