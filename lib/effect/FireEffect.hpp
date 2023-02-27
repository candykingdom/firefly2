#ifndef __FIRE_EFFECT_HPP__
#define __FIRE_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

class FireEffect : public Effect {
 public:
  FireEffect();

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) override;

 private:
  // Fire color palette.
  ColorPalette palette = {
      {0, 255, 8}, {23, 249, 45}, {30, 246, 113}, {35, 200, 192}};

  // Random time offset per device.
  uint16_t offset = 0;
};
#endif
