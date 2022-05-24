#ifndef __COLOR_CYCLE_EFFECT_HPP__
#define __COLOR_CYCLE_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** Cycles all of the LEDs through the color palette at once. */
class ColorCycleEffect : public Effect {
 public:
  ColorCycleEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;
};
#endif
