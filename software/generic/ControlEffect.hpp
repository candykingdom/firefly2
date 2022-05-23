#ifndef __CONTROL_EFFECT_HPP__
#define __CONTROL_EFFECT_HPP__

#include "Effect.hpp"
#include <Types.hpp>

/** A generic effect that can be used by external controllers. */
class ControlEffect : public Effect {
 public:
  ControlEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};
#endif
