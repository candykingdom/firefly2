#include "RorschachEffect.hpp"

#include "ColorPalette.hpp"
#include "Perlin.hpp"

RorschachEffect::RorschachEffect(const DeviceDescription *device)
    : Effect(device) {
#ifdef ARDUINO
  random16_set_seed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random16();
}

CRGB RorschachEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                             RadioPacket *setEffectPacket) {
  const uint8_t paletteIndex = setEffectPacket->readPaletteIndexFromSetEffect();
  const ColorPalette palette = palettes[paletteIndex];

  // LEDs at the center of the strip have a lower position.
  const uint16_t led_pos = -abs(ledIndex - (device->virtual_leds >> 1));

  timeMs += offset;
  uint16_t noise =
      perlinNoise((led_pos << 5) + (timeMs >> 3),
                  (timeMs >> 3) + (led_pos << 2));  // Skew coordinates to avoid
                                                    // moire patterns.

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
    if (device->type == DeviceType::Wearable) {
      color.v /= 2;
    }
    return color;
  }
}
