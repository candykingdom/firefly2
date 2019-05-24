#ifndef __EFFECT_HPP__
#define __EFFECT_HPP__

#include <vector>
#include "ColorPalette.hpp"
#include "Radio.hpp"
#include "Types.hpp"

class Effect {
 public:
  Effect(uint8_t numLeds);

  /** Gets the value of a specific LED at a specific time. */
  virtual CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
                      RadioPacket *setEffectPacket) = 0;

  const std::vector<ColorPalette> palettes = {
      // Solid color
      {CHSV(HUE_RED, 255, 255)},
      {CHSV(HUE_ORANGE, 255, 255)},
      {CHSV(HUE_YELLOW, 255, 255)},
      {CHSV(HUE_GREEN, 255, 255)},
      {CHSV(HUE_AQUA, 255, 255)},
      {CHSV(HUE_BLUE, 255, 255)},
      {CHSV(HUE_PURPLE, 255, 255)},
      {CHSV(HUE_PINK, 255, 255)},

      // Rainbow
      {CHSV(HUE_RED, 255, 255), CHSV(HUE_GREEN, 255, 255),
       CHSV(HUE_BLUE, 255, 255)},
      // Warm
      {CHSV(HUE_RED, 255, 255), CHSV(HUE_PURPLE, 255, 255)},
      // Cool
      {CHSV(HUE_GREEN, 255, 255), CHSV(HUE_BLUE, 255, 255)},
      // Yellow-green
      {CHSV(HUE_YELLOW, 255, 255), CHSV(HUE_AQUA, 255, 255)},
      // 80s Miami
      {CHSV(HUE_YELLOW, 255, 255), CHSV(HUE_ORANGE, 255, 255),
       CHSV(HUE_PURPLE, 255, 255)},
      // Vaporwave
      // https://i.redd.it/aepphltiqy911.png
      {CHSV(47, 255, 255), CHSV(349, 255, 255), CHSV(284, 255, 255)},
      // Popo
      {CHSV(HUE_RED, 255, 255), CHSV(HUE_BLUE, 255, 255)},
  };

 protected:
  const uint8_t numLeds;
};
#endif
