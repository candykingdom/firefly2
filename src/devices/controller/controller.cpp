#include <FastLED.h>

#include "Arduino.h"

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

void setup() { FastLED.addLeds<NEOPIXEL, kNeopixelPin>(leds, kLedCount); }

bool output = false;
void loop() {
  int button = ReadAnalogButton(kSwitchLeft);
  switch (button) {
    case -1:
      FastLED.showColor(CRGB::Black);
      break;

    case 0:
      FastLED.showColor(CRGB::Red);
      break;

    case 1:
      FastLED.showColor(CRGB::Green);
      break;

    case 2:
      FastLED.showColor(CRGB::Blue);
      break;
  }
  // FastLED.showColor(CHSV(millis() / 10, 255, 64));

  // delay(10);
}