#ifndef __COLOR_CYCLE_EFFECT_HPP__
#define __COLOR_CYCLE_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** Cycles all of the LEDs through the color palette at once. */
class ColorCycleEffect : public Effect {
 public:
  ColorCycleEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint16_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;
};
#endif
