#ifndef __DISPLAY_COLOR_PALETTE_EFFECT_HPP__
#define __DISPLAY_COLOR_PALETTE_EFFECT_HPP__

#include "../types/Types.hpp"
#include "Effect.hpp"

/** Utility effect that just displays a color palette across all of the LEDs. */
class DisplayColorPaletteEffect : public Effect {
 public:
  DisplayColorPaletteEffect(const DeviceDescription *device);

  /** Gets the value of a specific LED at a specific time. */
  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              RadioPacket *setEffectPacket) override;

 private:
};
#endif
