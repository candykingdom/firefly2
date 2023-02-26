#include <FastLED.h>

#include "Arduino.h"
#include "FakeLedManager.hpp"
#include "arduino/RadioHeadRadio.hpp"
#include "generic/NetworkManager.hpp"
#include "generic/RadioStateMachine.hpp"

// Pin definitions
constexpr int kSwitchLeft = PA0;
constexpr int kSwitchRight = PA1;
constexpr int kSwitchAction = PA2;

constexpr int kVbattDiv = PA3;

constexpr int kNeopixelPin = PB15;

// Constants for the analog switch inputs. See
// http://www.ignorantofthings.com/2018/07/the-perfect-multi-button-input-resistor.html?m=1
constexpr uint16_t kSwitch1Threshold = 346 / 2;
constexpr uint16_t kSwitch2Threshold = (346 + 678) / 2;
constexpr uint16_t kSwitch3Threshold = (678 + 1023) / 2;

constexpr uint16_t kLedCount = 42;
CRGB leds[kLedCount];

RadioHeadRadio radio;
NetworkManager nm(&radio);
RadioStateMachine state_machine(&nm);
FakeLedManager led_manager(
    DeviceDescription(2000, {StripDescription(/*led_count=*/12, {Bright})}),
    &state_machine);

// Returns which analog button is pressed on the given pin, or -1 if no button
// is pressed.
int ReadAnalogButton(int pin) {
  uint16_t input = analogRead(pin);
  if (input < kSwitch1Threshold) return 0;
  if (input < kSwitch2Threshold) return 1;
  if (input < kSwitch3Threshold) return 2;

  return -1;
}

void setup() {
  FastLED.addLeds<NEOPIXEL, kNeopixelPin>(leds, kLedCount)
      .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(64);

  if (!radio.Begin()) {
    while (true) {
      leds[0] = CRGB((millis() / 200) % 2 == 0 ? 255 : 0, 0, 0);
      FastLED.show();
    }
  }
  FastLED.clear(/*writeData=*/true);
}

void loop() {
  state_machine.Tick();
  led_manager.RunEffect();

  for (uint8_t i = 0; i < 12; i++) {
    leds[i] = led_manager.GetLed(i);
  }
  leds[37] = state_machine.GetCurrentState() == RadioState::Slave
                 ? CRGB(255, 0, 0)
                 : CRGB(0, 255, 0);
  FastLED.show();

  delay(5);
}
