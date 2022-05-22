#ifndef __SWINGING_LIGHTS_EFFECT_HPP__
#define __SWINGING_LIGHTS_EFFECT_HPP__

#include "Effect.hpp"
#include <Types.hpp>

class SwingingLights : public Effect {
 public:
  SwingingLights(DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  uint32_t period = 5000;  // 5 second period.
};
#endif
