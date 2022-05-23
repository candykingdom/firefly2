#ifndef __RAINBOW_BUMPS_EFFECT_HPP__
#define __RAINBOW_BUMPS_EFFECT_HPP__

#include "Effect.hpp"
#include <Types.hpp>

class RainbowBumpsEffect : public Effect {
 public:
  RainbowBumpsEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
