#include "analog-button.h"

AnalogButton::AnalogButton(int pin)
    : pin_(pin),
      analog_filter_{filter_functions::ForAnalogReadDynamic(pin),
                     kFilterAlpha} {
  analog_filter_.SetMinRunInterval(5);
}

void AnalogButton::Tick() {
  button1_pressed_ = false;
  button2_pressed_ = false;
  button3_pressed_ = false;

  analog_filter_.Run();
  int state = ReadAnalog();
  if (state != prev_state_ && analog_filter_.Stable(kStableThreshold)) {
    if (state == 1) {
      button1_pressed_ = true;
    } else if (state == 2) {
      button2_pressed_ = true;
    } else if (state == 3) {
      button3_pressed_ = true;
    }
    prev_state_ = state;
  }
}

bool AnalogButton::Button1Pressed() const { return button1_pressed_; }

bool AnalogButton::Button2Pressed() const { return button2_pressed_; }

bool AnalogButton::Button3Pressed() const { return button3_pressed_; }

int AnalogButton::ReadAnalog() const {
  uint16_t input = analog_filter_.GetFilteredValue();
  if (input < kSwitch1Threshold) return 1;
  if (input < kSwitch2Threshold) return 2;
  if (input < kSwitch3Threshold) return 3;

  return 0;
}