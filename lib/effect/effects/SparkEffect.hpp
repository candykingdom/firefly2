#ifndef __SPARK_EFFECT_HPP__
#define __SPARK_EFFECT_HPP__

#include "../../types/Types.hpp"
#include "../Effect.hpp"

// Makes a bright light trace back and forth, with a tail.
class SparkEffect : public Effect {
 public:
  SparkEffect(uint8_t numLeds);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
  const std::vector<uint8_t> brightnesses = {255, 255, 128, 96, 48, 32, 24, 16};
};
#endif
