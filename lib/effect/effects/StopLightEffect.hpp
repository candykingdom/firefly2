#ifndef __STOP_LIGHT_EFFECT_HPP__
#define __STOP_LIGHT_EFFECT_HPP__

#include "../../types/Types.hpp"
#include "../Effect.hpp"

class StopLightEffect : public Effect {
 public:
  StopLightEffect(uint8_t numLeds);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  const CHSV red{HUE_RED, 255, 255};
  const CHSV dimRed{HUE_RED, 255, 32};
  const CHSV amber{32, 255, 255};
  const CHSV dimAmber{32, 255, 32};
  const CHSV green{102, 255, 255};
  const CHSV dimGreen{102, 255, 32};
};
#endif
