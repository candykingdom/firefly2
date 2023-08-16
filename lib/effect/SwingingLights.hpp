#ifndef __SWINGING_LIGHTS_EFFECT_HPP__
#define __SWINGING_LIGHTS_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class SwingingLights : public Effect {
 public:
  SwingingLights();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint16_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;

 private:
  static constexpr uint32_t kPeriod = 5000;  // 5 second period.
};
#endif
