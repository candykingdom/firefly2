#ifndef __SIMPLE_BLINK_EFFECT_HPP__
#define __SIMPLE_BLINK_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** Cycles all of the LEDs through the color palette at once. */
class SimpleBlinkEffect : public Effect {
 public:
  SimpleBlinkEffect(const DeviceDescription *device, uint16_t speed);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              RadioPacket *setEffectPacket) override;

 private:
  const uint16_t speed;
};
#endif
