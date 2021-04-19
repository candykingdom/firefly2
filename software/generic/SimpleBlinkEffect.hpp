#ifndef __SIMPLE_BLINK_EFFECT_HPP__
#define __SIMPLE_BLINK_EFFECT_HPP__

#include <Effect.hpp>
#include <Types.hpp>

/** Cycles all of the LEDs through the color palette at once. */
class SimpleBlinkEffect : public Effect {
 public:
  SimpleBlinkEffect(uint8_t numLeds, DeviceType deviceType, uint16_t speed);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  const uint16_t speed;
};
#endif
