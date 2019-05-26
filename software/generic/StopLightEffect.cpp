#include "StopLightEffect.hpp"

StopLightEffect::StopLightEffect(uint8_t numLeds) : Effect(numLeds) {}

CRGB StopLightEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                             RadioPacket *setEffectPacket) {
  const uint16_t led_pos = abs((numLeds >> 1) - ledIndex) << 8;
  const uint16_t segment = numLeds << (8 - 3);
  timeMs = timeMs >> 11;

  const bool is_red = (timeMs & 0b100) == 0 && (timeMs & 0b11) > 0;
  const bool is_amber = (timeMs & 0b100) == 0 && (timeMs & 0b11) == 0;
  const bool is_green = (timeMs & 0b100);

  if (numLeds < 16) {
    if (is_red) {
      return red;
    } else if (is_amber) {
      return amber;
    } else {
      return green;
    }
  }

  if (led_pos < segment) {
    return {0, 0, 0};
  } else if (led_pos > segment && led_pos < segment * 2 && is_red) {
    return red;
  } else if (led_pos > segment * 2 && led_pos < segment * 3 && is_amber) {
    return amber;
  } else if (led_pos > segment * 3 && is_green) {
    return green;
  } else {
    return {0, 0, 0};
  }
}
