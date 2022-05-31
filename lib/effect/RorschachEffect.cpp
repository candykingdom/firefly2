#include "RorschachEffect.hpp"

#include <ColorPalette.hpp>
#include <Perlin.hpp>

RorschachEffect::RorschachEffect() : Effect() {
#ifdef ARDUINO
  random16_set_seed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random16();
}

CRGB RorschachEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                             const StripDescription *strip,
                             RadioPacket *setEffectPacket) {
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  const ColorPalette palette = palettes[palette_index];

  // LEDs at the center of the strip have a lower position.
  const uint16_t led_pos = -abs(led_index - (strip->led_count >> 1));

  time_ms += offset;
  uint16_t noise =
      perlinNoise((led_pos << 5) + (time_ms >> 3),
                  (time_ms >> 3) + (led_pos << 2));  // Skew coordinates to
                                                     // avoid moire patterns.

  // If the palette is only one color, change the value instead of the hue.
  if (palette.Size() < 2) {
    if (noise > MAX_UINT8) {
      noise = MAX_UINT8;
    }

    CHSV color = palette.GetColor(0);
    color.v = noise;
    return color;
  } else {
    CHSV color = palette.GetGradient(noise << 8, false);
    if (!strip->FlagEnabled(Bright)) {
      color.v /= 2;
    }
    return color;
  }
}
