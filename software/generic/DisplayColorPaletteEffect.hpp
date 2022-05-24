#ifndef __DISPLAY_COLOR_PALETTE_EFFECT_HPP__
#define __DISPLAY_COLOR_PALETTE_EFFECT_HPP__

#include <Types.hpp>

#include "Effect.hpp"

/** Utility effect that just displays a color palette across all of the LEDs. */
class DisplayColorPaletteEffect : public Effect {
 public:
  DisplayColorPaletteEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t ledIndex, uint32_t timeMs,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
