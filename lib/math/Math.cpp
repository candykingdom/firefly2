#include "Math.hpp"

#include <Types.hpp>
#include <cstdio>

void GetPosOnCircle(uint16_t led_count, uint16_t led_index, uint8_t *angle,
                    uint8_t *radius) {
  uint32_t tau = 6283;
  uint32_t circum = led_count * 32;
  *angle = led_index * 255 / led_count;
  *radius = (circum * 1000) / tau;
}

void MirrorIndex(uint16_t *led_index, uint16_t *led_count) {
  *led_index %= *led_count;

  uint16_t new_count = (*led_count + 1) / 2;

  if (*led_index >= new_count) {
    *led_index = *led_count - *led_index - 1;
  }

  *led_count = new_count;
}
