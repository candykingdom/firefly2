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
  CRGBPalette256 palette = CRGBPalette256(CRGB(0x000000), CRGB(0x2d0c01),
                                          CRGB(0x714004), CRGB(0xfd780c));

  // Random time offset per device.
  uint16_t offset = 0;
};
#endif
