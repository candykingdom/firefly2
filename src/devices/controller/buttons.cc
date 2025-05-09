#include "buttons.h"

void SetLeftButtonLed(uint8_t button_index, uint8_t brightness) {
  switch (button_index) {
    case 2:
      leds[kLeftButtonLed].r = brightness;
      break;

    case 1:
      leds[kLeftButtonLed].g = brightness;
      break;

    case 3:
      leds[kLeftButtonLed].b = brightness;
      break;
  }
}

void SetLeftButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3) {
  SetLeftButtonLed(1, button1);
  SetLeftButtonLed(2, button2);
  SetLeftButtonLed(3, button3);
}

void SetRightButtonLed(uint8_t button_index, uint8_t brightness) {
  switch (button_index) {
    case 2:
      leds[kRightButtonLed].r = brightness;
      break;

    case 3:
      leds[kRightButtonLed].g = brightness;
      break;

    case 1:
      leds[kRightButtonLed].b = brightness;
      break;
  }
}

void SetRightButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3) {
  SetRightButtonLed(1, button1);
  SetRightButtonLed(2, button2);
  SetRightButtonLed(3, button3);
}

void SetBottomButtonLed(uint8_t button_index, uint8_t brightness) {
  switch (button_index) {
    case 1:
      leds[kBottomButtonLed].r = brightness;
      break;

    case 2:
      leds[kBottomButtonLed].g = brightness;
      break;

    case 3:
      leds[kBottomButtonLed].b = brightness;
      break;
  }
}

void SetBottomButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3) {
  SetBottomButtonLed(1, button1);
  SetBottomButtonLed(2, button2);
  SetBottomButtonLed(3, button3);
}
