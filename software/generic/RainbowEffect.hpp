#ifndef __RAINBOW_EFFECT_HPP__
#define __RAINBOW_EFFECT_HPP__

#include "Effect.hpp"
#include "Types.hpp"

class RainbowEffect : public Effect {
 public:
  RainbowEffect(uint8_t numLeds, CRGB &color);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  CRGB color;
};
#endif
