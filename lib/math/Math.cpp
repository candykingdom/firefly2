#include "Math.hpp"

#include <cstdio>

#include "../types/Types.hpp"

void GetPosOnCircle(uint8_t led_count, uint8_t led_index, uint8_t *angle,
                    uint8_t *radius) {
  uint32_t tau = 6283;
  uint32_t circum = led_count * 32;
  *angle = led_index * 255 / led_count;
  *radius = (circum * 1000) / tau;
}

void MirrorIndex(uint8_t *led_index, uint8_t *led_count) {
  *led_index %= *led_count;

  uint8_t new_count = (*led_count + 1) / 2;

  if (*led_index >= new_count) {
    *led_index = *led_count - *led_index - 1;
  }

  *led_count = new_count;
}
