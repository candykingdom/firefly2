#ifndef __PRIDE_EFFECT__HPP__
#define __PRIDE_EFFECT__HPP__

#include <Types.hpp>

#include "Effect.hpp"

class PrideEffect : public Effect {
 public:
  PrideEffect();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription *strip,
              RadioPacket *setEffectPacket) override;

 private:
  // Pride color palette.
  ColorPalette palette = {
      // {36 * 255 / 360, 82 * 255 / 100, 47 * 255 / 100},
      {0 * 255 / 360, 100 * 255 / 100, 100 * 255 / 100},
      {34 * 255 / 360, 100 * 255 / 100, 99 * 255 / 100},
      {54 * 255 / 360, 100 * 255 / 100, 100 * 255 / 100},
      {118 * 255 / 360, 93 * 255 / 100, 63 * 255 / 100},
      {218 * 255 / 360, 97 * 255 / 100, 69 * 255 / 100},
      {292 * 255 / 360, 79 * 255 / 100, 86 * 255 / 100},
  };
};
#endif
