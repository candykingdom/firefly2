#ifndef __CONTROL_EFFECT_HPP__
#define __CONTROL_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** A generic effect that can be used by external controllers. */
class ControlEffect : public Effect {
 public:
  ControlEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;

 private:
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};
#endif
