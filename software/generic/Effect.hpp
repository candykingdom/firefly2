#ifndef __EFFECT_HPP__
#define __EFFECT_HPP__

#include "Types.hpp"

class Effect {
 public:
  Effect(uint8_t numLeds);

  /** Gets the value of a specific LED at a specific time. */
  virtual CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs) = 0;

 protected:
  const uint8_t numLeds;
};
#endif
