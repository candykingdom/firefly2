#ifndef __MITXELA_PIERCING_HPP__
#define __MITXELA_PIERCING_HPP__

#include <Types.hpp>

#include "Effect.hpp"

// TODO: describe the effect.
class MitxelaPiercing : public Effect {
 public:
  MitxelaPiercing();

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;
};
#endif
