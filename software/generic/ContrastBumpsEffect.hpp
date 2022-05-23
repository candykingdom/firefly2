#ifndef __CONTRAST_BUMPS_EFFECT_HPP__
#define __CONTRAST_BUMPS_EFFECT_HPP__

#include "Effect.hpp"
#include <Types.hpp>

// This has a background of one color, and smooth bumps of the opposite (e.g.
// halfway around) color in the palette.
class ContrastBumpsEffect : public Effect {
 public:
  ContrastBumpsEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
