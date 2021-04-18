#ifndef __RORSCHACH_EFFECT_HPP__
#define __RORSCHACH_EFFECT_HPP__

#include "Effect.hpp"
#include <Types.hpp>

class RorschachEffect : public Effect {
 public:
  RorschachEffect(uint8_t numLeds, DeviceType deviceType);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  // Random time offset per device.
  uint16_t offset = 0;
};
#endif
