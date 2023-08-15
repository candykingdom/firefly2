#ifndef __RAINBOW_BUMPS_EFFECT_HPP__
#define __RAINBOW_BUMPS_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class RainbowBumpsEffect : public Effect {
 public:
  RainbowBumpsEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint16_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;

 private:
};
#endif
