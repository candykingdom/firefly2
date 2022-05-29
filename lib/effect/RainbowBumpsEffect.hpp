#ifndef __RAINBOW_BUMPS_EFFECT_HPP__
#define __RAINBOW_BUMPS_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class RainbowBumpsEffect : public Effect {
 public:
  RainbowBumpsEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
