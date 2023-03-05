#include "FireflyEffect.hpp"

#include <Debug.hpp>

FireflyEffect::FireflyEffect() : Effect() {
#ifdef ARDUINO
  randomSeed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random(0, kBlinkPeriod / 2);
}

CRGB FireflyEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                           const StripDescription &strip,
                           RadioPacket *setEffectPacket) {
  if (strip.FlagEnabled(Controller)) {
    // For the controller, blink the lights mostly in sync
    if (led_index % 2 == 1) {
      return CRGB(0, 0, 0);
    }

    offset = ((kBlinkPeriod + 1234) << led_index) % (kBlinkPeriod / 2);
  }

  const int8_t phase = (time_ms / kPeriodMs) % 3;

  uint32_t adjusted_time = 0;
  const uint32_t period_start = (time_ms / kPeriodMs) * kPeriodMs;
  const uint32_t elapsed_in_period = time_ms - period_start;
  const uint32_t remaining_in_period = kPeriodMs - elapsed_in_period;
  if (phase == 0) {
    // Out of sync -> in sync
    if (offset < kBlinkPeriod / 4) {
      adjusted_time = time_ms - (offset * remaining_in_period) / kPeriodMs;
    } else {
      adjusted_time = time_ms + (offset * remaining_in_period) / kPeriodMs;
    }
  } else if (phase == 1) {
    // In sync
    adjusted_time = time_ms;
  } else {
    // In sync -> out of sync
    if (offset < kBlinkPeriod / 4) {
      adjusted_time = time_ms - (offset * elapsed_in_period) / kPeriodMs;
    } else {
      adjusted_time = time_ms + (offset * elapsed_in_period) / kPeriodMs;
    }
  }

  int16_t curve = sin16(adjusted_time * kSinMultiplier);
  if (curve < 0) {
    curve = 0;
  }
  ColorPalette palette =
      palettes()[setEffectPacket->readPaletteIndexFromSetEffect()];
  CHSV color = palette.GetGradient((time_ms / kBlinkPeriod) << 8);
  color.v = curve / 256;
  return color;
}
