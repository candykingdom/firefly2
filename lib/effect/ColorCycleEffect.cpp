#include "ColorCycleEffect.hpp"

#include "../../software/generic/ColorPalette.hpp"

ColorCycleEffect::ColorCycleEffect(const DeviceDescription *device)
    : Effect(device){};

CRGB ColorCycleEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                              RadioPacket *setEffectPacket) {
  const uint8_t palette_index =
      setEffectPacket->readPaletteIndexFromSetEffect();
  const ColorPalette palette = palettes[palette_index];
  // Check for whether the entire palette is the same color - if so, change the
  // brightness rather than the hue.
  if (palette.Size() < 2) {
    // Solid color palette
    CHSV color = palette.GetColor(0);
    if (device->FlagEnabled(Bright)) {
      color.v = ((uint16_t)cubicwave8(time_ms / 16));
    } else {
      color.v = ((uint16_t)cubicwave8(time_ms / 16)) * 2 / 3;
    }
    return color;
  } else {
    CHSV color = palette.GetGradient((time_ms / 16) << 8);
    if (!device->FlagEnabled(Bright)) {
      color.v /= 2;
    }
    return color;
  }
}
