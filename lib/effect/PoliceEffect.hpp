#ifndef __POLICE_EFFECT_HPP__
#define __POLICE_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class PoliceEffect : public Effect {
 public:
  PoliceEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;

 private:
  static constexpr uint8_t light_width = 14;
  static constexpr uint8_t red_speed = 15;
  static constexpr uint8_t blue_speed = 14;

  const CRGB red = CRGB(255, 0, 0);
  const CRGB blue = CRGB(0, 0, 255);
  const CRGB off = CRGB(0, 0, 0);
};
#endif
