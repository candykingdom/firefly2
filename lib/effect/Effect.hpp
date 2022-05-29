#ifndef __EFFECT_HPP__
#define __EFFECT_HPP__

#include <ColorPalette.hpp>
#include <DeviceDescription.hpp>
#include <Radio.hpp>
#include <Types.hpp>
#include <vector>

class Effect {
 public:
  Effect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  virtual CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
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

  const DeviceDescription *const device;
};
#endif
