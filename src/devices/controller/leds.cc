#include "leds.h"

std::array<CRGB, kLedCount> leds;

// Sets one of the 3 x 12 LEDs, in left-to-right top-to-bottom order.
void SetMainLed(uint8_t led_index, CRGB rgb) {
  if (led_index > 11 && led_index < 24) {
    leds[12 + (23 - led_index)] = rgb;
  } else {
    leds[led_index] = rgb;
  }
}
