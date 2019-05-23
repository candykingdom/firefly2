#ifndef __POLICE_EFFECT_HPP__
#define __POLICE_EFFECT_HPP__

#include "Effect.hpp"
#include "Types.hpp"

class PoliceEffect : public Effect {
 public:
  PoliceEffect(uint8_t numLeds);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  const uint8_t lightWidth = 14;
  const uint8_t redSpeed = 15;
  const uint8_t blueSpeed = 14;

  const CRGB red = CRGB(255, 0, 0);
  const CRGB blue = CRGB(0, 0, 255);
  const CRGB off = CRGB(0, 0, 0);
};
#endif
