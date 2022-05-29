#include "FastLedManager.hpp"

#include "../device/DeviceDescription.hpp"

FastLedManager::FastLedManager(const DeviceDescription *device,
                               RadioStateMachine *radio_state)
    : LedManager(device, radio_state) {
  // The first LED is on-board, and should mirror the first LED of the strip.
  leds = new CRGB[device->led_count + 1];
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, device->led_count + 1)
      .setCorrection(TypicalLEDStrip);
  FastLED.showColor(CRGB(0, 0, 0));
}

void FastLedManager::SetGlobalColor(CRGB rgb) { FastLED.showColor(rgb); }

void FastLedManager::PlayStartupAnimation(bool error) {
  CRGB color;
  if (error) {
    color = CRGB(128, 0, 0);
  } else {
    color = CRGB(128, 128, 128);
  }

  for (uint8_t i = 0; i < device->led_count * 2; ++i) {
    uint8_t index = i;
    if (i >= device->led_count) {
      index = device->led_count - (i - device->led_count);
    }
    FastLED.clear();
    SetLed(index, &color);
    FastLED.show();
    delay(500 / device->led_count);
  }
}

void FastLedManager::SetLed(uint8_t led_index, CRGB *const rgb) {
  // The first LED is on-board, and should mirror the first LED of the strip.
  if (led_index == 0) {
    leds[0] = *rgb;
  }
  leds[led_index + 1] = *rgb;
}

void FastLedManager::WriteOutLeds() { FastLED.show(); }
