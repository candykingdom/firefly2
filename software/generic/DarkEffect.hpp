#ifndef __DARK_EFFECT__HPP__
#define __DARK_EFFECT__HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** Utility effect that turns off all LEDs. */
class DarkEffect : public Effect {
 public:
  DarkEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
