#include "FastLedManager.hpp"

FastLedManager::FastLedManager(DeviceDescription *device,
                               RadioStateMachine *radioState)
    : LedManager(device, radioState) {
  // The first LED is on-board, and should mirror the first LED of the strip.
  leds = new CRGB[device->physicalLeds + 1];
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, device->physicalLeds + 1)
      .setCorrection(TypicalLEDStrip);
  FastLED.showColor(CRGB(0, 0, 0));
}

void FastLedManager::SetGlobalColor(CRGB rgb) { FastLED.showColor(rgb); }

void FastLedManager::SetLed(uint8_t ledIndex, CRGB *const rgb) {
  // The first LED is on-board, and should mirror the first LED of the strip.
  if (ledIndex == 0) {
    leds[0] = *rgb;
  }
  leds[ledIndex + 1] = *rgb;
}

void FastLedManager::WriteOutLeds() { FastLED.show(); }
