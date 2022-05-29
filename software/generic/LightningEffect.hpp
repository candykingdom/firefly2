#ifndef __LIGHTNING_EFFECT_HPP__
#define __LIGHTNING_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

// Blinks groups of LEDS in a vaguely lightning-light pattern.
class LightningEffect : public Effect {
 public:
  LightningEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
