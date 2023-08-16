#include "StopLightEffect.hpp"

StopLightEffect::StopLightEffect() : Effect() {}

CRGB StopLightEffect::GetRGB(uint16_t led_index, uint32_t time_ms,
                             const StripDescription &strip,
                             RadioPacket *setEffectPacket) const {
  UNUSED(setEffectPacket);
  const uint16_t led_pos = abs((strip.led_count >> 1) - led_index) << 8;
  time_ms = time_ms >> 11;

  const bool is_red = (time_ms & 0b100) == 0 && (time_ms & 0b11) > 0;
  const bool is_amber = (time_ms & 0b100) == 0 && (time_ms & 0b11) == 0;
  const bool is_green = (time_ms & 0b100);

  if (strip.FlagEnabled(Tiny)) {
    if (is_red) {
      return red;
    } else if (is_amber) {
      return amber;
    } else {
      return green;
    }
  }

  if (strip.FlagEnabled(Controller)) {
    const uint16_t segment = strip.led_count / 5;
    if (led_index <= segment) {
      return red;
    }
    if (led_index > (segment * 2) && led_index <= (segment * 3)) {
      return amber / 2;
    }
    if (led_index > (segment * 4)) {
      return green;
    }
    return CRGB(0, 0, 0);
  }

  const uint16_t segment = strip.led_count << (8 - 3);

  if (led_pos < segment) {
    return {0, 0, 0};
  } else if (led_pos > segment && led_pos < segment * 2) {
    if (is_red) {
      return red;
    } else {
      return dimRed;
    }
  } else if (led_pos > segment * 2 && led_pos < segment * 3) {
    if (is_amber) {
      return amber;
    } else {
      return dimAmber;
    }
  } else if (led_pos > segment * 3) {
    if (is_green) {
      return green;
    } else {
      return dimGreen;
    }
  } else {
    return {0, 0, 0};
  }
}
