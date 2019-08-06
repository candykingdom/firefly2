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
      {{HUE_RED, 255, 255}},
      {{HUE_ORANGE, 255, 255}},
      {{HUE_YELLOW, 255, 255}},
      {{HUE_GREEN, 255, 255}},
      {{HUE_AQUA, 255, 255}},
      {{HUE_BLUE, 255, 255}},
      {{HUE_PURPLE, 255, 255}},
      {{HUE_PINK, 255, 255}},

      // Rainbow
      {{HUE_RED, 255, 255}, {HUE_GREEN, 255, 255}, {HUE_BLUE, 255, 255}},
      // Warm
      {{HUE_RED, 255, 255}, {HUE_PURPLE, 255, 255}},
      // Cool
      {{HUE_GREEN, 255, 255}, {HUE_BLUE, 255, 255}},
      // Yellow-green
      {{HUE_YELLOW, 255, 255}, {HUE_AQUA, 255, 255}},
      // 80s Miami
      {{HUE_PURPLE, 255, 255}, {HUE_ORANGE, 255, 255}},
      // Vaporwave
      // https://i.redd.it/aepphltiqy911.png
      {{33, 241, 249}, {247, 188, 255}, {201, 225, 160}, {153, 251, 150}},
      // Popo
      {{HUE_RED, 255, 255}, {HUE_BLUE, 255, 255}},
      // Candy-cane
      {{HUE_RED, 255, 255}, {HUE_RED, 0, 255}},
      // Winter-mint candy-cane
      {{HUE_AQUA, 255, 255}, {HUE_AQUA, 0, 255}},
      // Fire
      {{HUE_RED, 255, 255}, {HUE_ORANGE, 255, 255}, {HUE_YELLOW, 255, 255}},
      // Pastel rainbow
      {{HUE_RED, 127, 192}, {HUE_GREEN, 127, 192}, {HUE_BLUE, 127, 192}},
      // Jazz cup - teal, purple, white
      {{132, 255, 255}, {192, 255, 255}, {0, 0, 200}},
      // Yellow and double purp
      {{HUE_PURPLE, 255, 255}, {HUE_YELLOW, 255, 255}, {HUE_PURPLE, 255, 255}},
      // Double rainbow (makes effects that depend on the number of colors, like
      // swinging lights, do cool things)
      {{HUE_RED, 255, 255},
       {HUE_GREEN, 255, 255},
       {HUE_BLUE, 255, 255},
       {HUE_RED, 255, 255},
       {HUE_GREEN, 255, 255},
       {HUE_BLUE, 255, 255}},
  };

 protected:
  const uint8_t numLeds;
};
#endif
