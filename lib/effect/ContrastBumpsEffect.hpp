#ifndef __CONTRAST_BUMPS_EFFECT_HPP__
#define __CONTRAST_BUMPS_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

// This has a background of one color, and smooth bumps of the opposite (e.g.
// halfway around) color in the palette.
class ContrastBumpsEffect : public Effect {
 public:
  ContrastBumpsEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
