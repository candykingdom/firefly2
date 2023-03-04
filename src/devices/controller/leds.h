#include <Types.hpp>

constexpr uint16_t kLedCount = 42;
extern CRGB leds[kLedCount];

// LED indices
constexpr uint8_t kStatusLeft = 39;
constexpr uint8_t kStatusMiddle = 38;
constexpr uint8_t kStatusRight = 37;

void SetMainLed(uint8_t led_index, CRGB rgb);
