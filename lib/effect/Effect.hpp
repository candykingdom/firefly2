#ifndef __EFFECT_HPP__
#define __EFFECT_HPP__

#include <ColorPalette.hpp>
#include <DeviceDescription.hpp>
#include <Radio.hpp>
#include <Types.hpp>
#include <vector>

class Effect {
 public:
  Effect();
  virtual ~Effect() = default;

  /** Gets the value of a specific LED at a specific time. */
  virtual CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
                      const StripDescription& strip,
                      RadioPacket* setEffectPacket) const = 0;

  static const std::vector<ColorPalette>& palettes();
  static const std::vector<const char*>& palette_names();

 protected:
  /**
   * Gets a sin wave, only considering the part above threshold, scaled to be
   * 0-255.
   *
   * Note that this means that the second (negative) half of the sine is always
   * 0.
   */
  uint8_t GetThresholdSin(int16_t x, uint8_t threshold) const;

  /**
   * Converts an interpolated palette color to RGB with the extra power that
   * hsv2rgb_rainbow pumps into hues near yellow (hue 32-95) scaled back out,
   * so a smooth gradient drives uniform total power. Without this, a
   * gradient crossing the yellow band drives up to ~34% more power than its
   * endpoints, which reads as a lone bright yellow LED between two palette
   * colors (e.g. the rainbow palette's red->green blend). Exact palette
   * colors rendered via GetColor keep FastLED's solid-color boost.
   */
  CRGB FlattenedGradientRGB(const CHSV& color) const;
};
#endif
