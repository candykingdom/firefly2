#ifndef __LIGHTNING_EFFECT_HPP__
#define __LIGHTNING_EFFECT_HPP__

#include "Effect.hpp"
#include <Types.hpp>

// Blinks groups of LEDS in a vaguely lightning-light pattern.
class LightningEffect : public Effect {
 public:
  LightningEffect(DeviceDescription *const device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
