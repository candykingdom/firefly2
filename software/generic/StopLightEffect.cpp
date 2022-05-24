#include "StopLightEffect.hpp"

StopLightEffect::StopLightEffect(const DeviceDescription *device)
    : Effect(device) {}

CRGB StopLightEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                             RadioPacket *setEffectPacket) {
  const uint16_t led_pos = abs((device->led_count >> 1) - ledIndex) << 8;
  const uint16_t segment = device->led_count << (8 - 3);
  timeMs = timeMs >> 11;

  const bool is_red = (timeMs & 0b100) == 0 && (timeMs & 0b11) > 0;
  const bool is_amber = (timeMs & 0b100) == 0 && (timeMs & 0b11) == 0;
  const bool is_green = (timeMs & 0b100);

  if (device->led_count < 16) {
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
