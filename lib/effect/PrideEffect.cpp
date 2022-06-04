#include "PrideEffect.hpp"

PrideEffect::PrideEffect() : Effect() {}

// The minimum number of steps of "temporal resolution" per pixel. Is multiplied
// by the strip LED count to ensure the result is a round number.
static const uint8_t depth_multiplier = 8;

// The reciprocal of the amount of fade per stripe width.
static const uint8_t fade_fract = 8;

CRGB PrideEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                         const StripDescription *strip,
                         RadioPacket *setEffectPacket) {
  const uint16_t stripe_width = strip->led_count * depth_multiplier;

  uint16_t fade_width = stripe_width / fade_fract;
  if (strip->FlagEnabled(Tiny)) {
    led_index = 0;
    time_ms *= 2;
    fade_width /= 2;
  }

  const uint16_t v_index =
      ((led_index * depth_multiplier * palette.Size()) + (time_ms / 8)) %
      (stripe_width * palette.Size());
  const uint8_t color_index = (v_index / stripe_width) % palette.Size();
  const uint16_t color_amount = (v_index % stripe_width);
  if (color_amount < fade_width) {
    return blend(palette.GetColor(color_index - 1),
                 palette.GetColor(color_index), color_amount * 255 / fade_width,
                 SHORTEST_HUES);
  } else {
    return palette.GetColor(color_index);
  }
}
