#ifndef __LIGHTNING_EFFECT_HPP__
#define __LIGHTNING_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

// Blinks groups of LEDS in a vaguely lightning-light pattern.
class LightningEffect : public Effect {
 public:
  LightningEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint16_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;

 private:
};
#endif
