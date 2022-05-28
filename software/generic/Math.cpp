#include "Math.hpp"

void GetPosOnCircle(uint8_t led_count, uint8_t led_index, uint8_t *angle,
                    uint8_t *radius) {
  uint32_t tau = 6283;
  uint32_t circum = led_count * 32;
  *angle = led_index * 255 / led_count;
  *radius = (circum * 1000) / tau;
}
