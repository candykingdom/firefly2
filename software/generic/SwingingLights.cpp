#include "SwingingLights.hpp"

static const int16_t MAX_SIGNED = 32767;
static const uint16_t MAX_UNSIGNED = 65535;

static const uint16_t SPREAD = MAX_UNSIGNED * 0.2;

SwingingLights::SwingingLights(uint8_t numLeds) : Effect(numLeds) {
  // This effect looks bad on less than 10 LEDs. Instead of creating another
  // effect we can just make the LEDs flash when a light pulse hits the end of a
  // "long" strip which looks pretty cool.
  if (numLeds < 10) {
    numLeds = 50;
  }
}

// Add a CHSV value to a CRGB in place.
static void addInPlace(const CHSV& value, CRGB& result) {
  CRGB color;
  hsv2rgb_rainbow(value, color);
  result.r = qadd8(result.r, color.r);
  result.g = qadd8(result.g, color.g);
  result.b = qadd8(result.b, color.b);
}

CRGB SwingingLights::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                            RadioPacket* setEffectPacket) {
  // Map [0, period) to [0, MAX_UNSIGNED)
  const int16_t angle = (timeMs % period) * ((float)MAX_UNSIGNED / period);

  // Map [0, numLeds) to [-MAX_SIGNED, MAX_SIGNED)
  const int16_t light_pos =
      (ledIndex - numLeds / 2) * ((float)MAX_SIGNED / (numLeds / 2));

  const int16_t first_pos = sin16(angle);
  const int16_t second_pos = -first_pos;

  const int32_t first_dist = abs(first_pos - light_pos) - SPREAD;
  const int32_t second_dist = abs(second_pos - light_pos) - SPREAD;

  CRGB color(0, 0, 0);

  CHSVPalette16 palette =
      palettes[setEffectPacket->readPaletteIndexFromSetEffect()];

  if (first_dist < 0) {
    CHSV modifier = palette[0];
    modifier.v = -first_dist * modifier.v / SPREAD;
    addInPlace(modifier, color);
  }

  if (second_dist < 0) {
    CHSV modifier = palette[15];
    modifier.v = -second_dist * modifier.v / SPREAD;
    addInPlace(modifier, color);
  }

  return color;
}
