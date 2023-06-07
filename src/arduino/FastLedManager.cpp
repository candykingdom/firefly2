#include "FastLedManager.hpp"

#include <DeviceDescription.hpp>
#include <algorithm>
#include <functional>
#include <numeric>

FastLedManager::FastLedManager(const DeviceDescription &device,
                               RadioStateMachine *radio_state)
    : LedManager(device, radio_state) {
  uint16_t led_count = device.GetLedCount();

  // The first LED is on-board, and only serves as a bit shift pass through.
  leds = new CRGB[led_count + 1];
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, led_count + 1)
      .setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, device.milliamps_supported);
  FastLED.clear(/*writeData=*/true);
}

void FastLedManager::SetGlobalColor(const CRGB &rgb) { FastLED.showColor(rgb); }

void FastLedManager::PlayStartupAnimation() {
  uint16_t led_count = device.GetLedCount();
 
  uint16_t half_delay = 500 / led_count;
  uint16_t min_delay = led_count / 30 + 1;

  CRGB white = CRGB(128, 128, 128);
  for (uint32_t i = 0; i < led_count; ++i) {
    FastLED.clear();
    SetLed(i, white);
    FastLED.show();
    delay(std::max(half_delay, min_delay));
  }

  FastLED.clear();
  for (uint32_t i = 0; i < 256; ++i) {
    CHSV col = CHSV(i, 255, 128);
    SetLed(led_count - 1, col);
    FastLED.show();
    delay(min_delay);
  }

  for (uint32_t i = led_count; i > 0; --i) {
    FastLED.clear();
    SetLed(i - 1, white);
    FastLED.show();
    delay(std::max(half_delay, min_delay));
  }

  FastLED.clear(/*writeData=*/true);
}

void FastLedManager::FatalErrorAnimation() {
  while (true) {
    FastLED.showColor(CRGB(64, 0, 0));
    delay(100);
    FastLED.showColor(CRGB(0, 0, 0));
    delay(100);
  }
}

void FastLedManager::SetLed(uint8_t led_index, const CRGB &rgb) {
  // If we only have one LED then treat the board LED as the first LED. This is
  // useful for testing boards themselves.
  if (device.GetLedCount() == 1 && led_index == 0) {
    leds[0] = rgb;
  }
  leds[led_index + 1] = rgb;
}

void FastLedManager::SetOnboardLed(const CRGB &rgb) { leds[0] = rgb; }

void FastLedManager::WriteOutLeds() { FastLED.show(); }
