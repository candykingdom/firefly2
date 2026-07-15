#ifndef __FIREFLY_EFFECT_HPP__
#define __FIREFLY_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** Lights blink - they smoothly change between in- and out-of-sync. */
class FireflyEffect : public Effect {
 public:
  FireflyEffect();

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;

 private:
  // The time it takes for the lights to go from randomly distributed to in-sync
  static constexpr uint32_t kPeriodMs = 20000;

  // Smaller number means longer blinks
  static constexpr uint32_t kSinMultiplier = 64;
  static constexpr uint32_t kBlinkPeriod = (1 << 16) / kSinMultiplier;

  // How far off from the 'true' pulse this light is (max)
  uint32_t offset_ = 0;
};
#endif
