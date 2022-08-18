#include "FastLedControllerManager.hpp"

#include <DeviceDescription.hpp>

const uint32_t CONTROLLER_BOARD_MA_SUPPORTED = 1000 - 50;

const DeviceDescription* device = new DeviceDescription(
    CONTROLLER_BOARD_MA_SUPPORTED, {
                                       new StripDescription(12, {}),
                                   });

FastLedControllerManager::FastLedControllerManager(
    RadioStateMachine* radio_state)
    : LedManager(device, radio_state) {
  leds = new CRGB[kControllerLedCount];
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, kControllerLedCount)
      .setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, device->milliamps_supported);
  FastLED.showColor(CRGB::Black);
}

void FastLedControllerManager::RunEffect() {
  std::fill_n(leds, kControllerLedCount, CRGB::Black);

  RadioPacket setEffectPacket;
  setEffectPacket.writeSetEffect(0, 0, palette_index);
  Effect* effect = GetEffect(effect_index);
  for (uint8_t index = 0; index < kControllerLightsPerRow; ++index) {
    CRGB rgb = effect->GetRGB(index, radio_state->GetNetworkMillis(),
                              device->strips[0], &setEffectPacket);
    rgb = CRGB::Red;
    SetLed(index, &rgb);
  }

  WriteOutLeds();
}

void FastLedControllerManager::SetGlobalColor(CRGB rgb) {
  FastLED.showColor(rgb);
}

void FastLedControllerManager::SetLed(uint8_t led_index, CRGB* const rgb) {
  leds[led_index] = *rgb;
}

void FastLedControllerManager::WriteOutLeds() { FastLED.show(); }
