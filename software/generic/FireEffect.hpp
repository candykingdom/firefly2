#ifndef __FIRE_EFFECT_HPP__
#define __FIRE_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class FireEffect : public Effect {
 public:
  FireEffect(const DeviceDescription *device);

  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  // Fire color palette.
  ColorPalette palette = {
      {0, 255, 8}, {23, 249, 45}, {30, 246, 113}, {35, 200, 192}};

  // Random time offset per device.
  uint16_t offset = 0;
};
#endif
