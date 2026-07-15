#pragma once

#include <Types.hpp>
#include <array>

constexpr uint16_t kLedCount = 42;
extern std::array<CRGB, kLedCount> leds;

// LED indices
constexpr uint8_t kStatusLeft = 7;
constexpr uint8_t kStatusMiddle = 8;
constexpr uint8_t kStatusRight = 9;

// Sets one of the 3 x 12 LEDs, in left-to-right top-to-bottom order.
void SetMainLed(uint8_t led_index, CRGB rgb);
