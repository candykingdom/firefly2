#ifndef __EFFECT_HPP__
#define __EFFECT_HPP__

#include <vector>

#include "ColorPalette.hpp"
#include <Radio.hpp>
#include <Types.hpp>

enum class DeviceType {
  Wearable,
  Bike,
};

class Effect {
 public:
  Effect(uint8_t numLeds);
  Effect(uint8_t numLeds, DeviceType deviceType);

  /** Gets the value of a specific LED at a specific time. */
  virtual CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
                      RadioPacket *setEffectPacket) = 0;

  static const std::vector<ColorPalette> palettes;

 protected:
  /**
   * Gets a sin wave, only considering the part above threshold, scaled to be
   * 0-255.
   *
   * Note that this means that the second (negative) half of the sine is always
   * 0.
   */
  uint8_t GetThresholdSin(int16_t x, uint8_t threshold);

  const uint8_t numLeds;
  const DeviceType deviceType;
};
#endif
