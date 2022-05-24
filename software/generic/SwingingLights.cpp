#include "SwingingLights.hpp"

#include "ColorPalette.hpp"

static const uint16_t SPREAD = MAX_UINT16 * 0.2;

SwingingLights::SwingingLights(const DeviceDescription *device)
    : Effect(device) {}

// Add a CHSV value to a CRGB in place.
static void addInPlace(const CHSV &value, CRGB &result) {
  CRGB color;
  hsv2rgb_rainbow(value, color);
  result.r = qadd8(result.r, color.r);
  result.g = qadd8(result.g, color.g);
  result.b = qadd8(result.b, color.b);
}

CRGB SwingingLights::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                            RadioPacket *setEffectPacket) {
  // This effect looks bad on small devices. Instead of creating another
  // effect we can just make the LEDs flash when a light pulse hits the end of a
  // "long" strip which looks pretty cool.
  uint8_t numLeds = device->led_count;
  if (device->FlagEnabled(Tiny) && !device->FlagEnabled(Circular)) {
    ledIndex = 0;
    numLeds = 50;
  }

  // Map [0, period) to [0, MAX_UINT16)
  const fract16 angle = (timeMs % period) * MAX_UINT16 / period;

  // Map [0, numLeds) to [0, MAX_UINT16)
  const fract16 led_pos = (uint32_t)ledIndex * MAX_UINT16 / numLeds;

  ColorPalette palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];

  CRGB color(0, 0, 0);

  for (uint32_t i = 0; i < palette.Size(); ++i) {
    const fract16 light_offset = i * MAX_UINT16 / palette.Size();
    const fract16 light_pos = sin16(light_offset + angle) + MAX_UINT16 / 2;

    int32_t dist = abs((int32_t)light_pos - led_pos) - SPREAD;

    if (dist < 0) {
      CHSV modifier = palette.GetColor(i);
      modifier.v = -dist * modifier.v / SPREAD;
      addInPlace(modifier, color);
    }
  }

  return color;
}
