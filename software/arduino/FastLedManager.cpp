#include "FastLedManager.hpp"

FastLedManager::FastLedManager(const DeviceDescription *device,
                               RadioStateMachine *radioState)
    : LedManager(device, radioState) {
  // The first LED is on-board, and should mirror the first LED of the strip.
  leds = new CRGB[device->physical_leds + 1];
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, device->physical_leds + 1)
      .setCorrection(TypicalLEDStrip);
  FastLED.showColor(CRGB(0, 0, 0));
}

void FastLedManager::SetGlobalColor(CRGB rgb) { FastLED.showColor(rgb); }

void FastLedManager::PlayStartupAnimation() {
  CRGB white = CRGB(128, 128, 128);
  for (uint8_t i = 0; i < device->physical_leds * 2; ++i) {
    uint8_t index = i;
    if (i >= device->physical_leds) {
      index = device->physical_leds - (i - device->physical_leds);
    }
    FastLED.clear();
    SetLed(index, &white);
    FastLED.show();
    delay(500 / device->physical_leds);
  }
}

void FastLedManager::SetLed(uint8_t ledIndex, CRGB *const rgb) {
  // The first LED is on-board, and should mirror the first LED of the strip.
  if (ledIndex == 0) {
    leds[0] = *rgb;
  }
  leds[ledIndex + 1] = *rgb;
}

void FastLedManager::WriteOutLeds() { FastLED.show(); }
