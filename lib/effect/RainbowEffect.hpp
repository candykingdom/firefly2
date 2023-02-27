#ifndef __RAINBOW_EFFECT_HPP__
#define __RAINBOW_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class RainbowEffect : public Effect {
 public:
  RainbowEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) override;
};
#endif
