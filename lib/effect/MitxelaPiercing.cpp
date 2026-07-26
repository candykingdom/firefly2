#include "MitxelaPiercing.hpp"

#include <Math.hpp>

MitxelaPiercing::MitxelaPiercing() : Effect() {}

CRGB MitxelaPiercing::GetRGB(uint8_t led_index, uint32_t time_ms,
                             const StripDescription& strip,
                             RadioPacket* setEffectPacket) const {
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  const ColorPalette& palette = PaletteAt(palette_index);

  uint8_t stripes = 6;
  const uint8_t stripe_idx = (led_index * stripes / strip.led_count) % stripes;
  CHSV color = palette.GetColor(stripe_idx);
  color.h += (cos8((uint32_t)time_ms * stripe_idx / 32) - 128) / 12;
  color.v = sin8((uint32_t)time_ms * (20 + stripe_idx) / 84);
  return color;
}
