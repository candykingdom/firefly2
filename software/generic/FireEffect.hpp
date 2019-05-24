#ifndef __FIRE_EFFECT_HPP__
#define __FIRE_EFFECT_HPP__

#include "Effect.hpp"
#include "Types.hpp"

class FireEffect : public Effect {
 public:
  FireEffect(uint8_t numLeds);

  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  // Fire color palette.
  ColorPalette palette = {
      {0, 0, 0}, {11, 249, 45}, {23, 246, 113}, {35, 125, 255}};

  // Random time offset per device.
  uint16_t offset = 0;
};
#endif
