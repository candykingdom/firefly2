#ifndef __FIREFLY_EFFECT_HPP__
#define __FIREFLY_EFFECT_HPP__

#include "Effect.hpp"
#include "Types.hpp"

/** Lights blink - they smoothly change between in- and out-of-sync. */
class FireflyEffect : public Effect {
 public:
  FireflyEffect(uint8_t numLeds);

  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  // The time it takes for the lights to go from randomly distributed to in-sync
  const uint32_t kPeriodMs = 20000;

  // Smaller number means longer blinks
  const uint32_t kSinMultiplier = 64;
  const uint32_t kBlinkPeriod = (1 << 16) / kSinMultiplier;

  // How far off from the 'true' pulse this light is (max)
  uint32_t offset = 0;
};
#endif
