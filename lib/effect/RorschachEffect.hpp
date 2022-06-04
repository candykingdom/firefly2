#ifndef __RORSCHACH_EFFECT_HPP__
#define __RORSCHACH_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class RorschachEffect : public Effect {
 public:
  RorschachEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription *strip,
              RadioPacket *setEffectPacket) override;

 private:
  // Random time offset per device.
  uint16_t offset = 0;
};
#endif
