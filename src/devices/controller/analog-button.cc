#include "analog-button.h"

#include <Arduino.h>

AnalogButton::AnalogButton(int pin) : pin_(pin) {}

void AnalogButton::Tick() {
  button1_pressed_ = false;
  button2_pressed_ = false;
  button3_pressed_ = false;

  int state = ReadAnalog();
  if (state != prev_state_) {
    if (state == 1) {
      button1_pressed_ = true;
    } else if (state == 2) {
      button2_pressed_ = true;
    } else if (state == 3) {
      button3_pressed_ = true;
    }
  }
  prev_state_ = state;
}

bool AnalogButton::Button1Pressed() { return button1_pressed_; }

bool AnalogButton::Button2Pressed() { return button2_pressed_; }

bool AnalogButton::Button3Pressed() { return button3_pressed_; }

int AnalogButton::ReadAnalog() {
  uint16_t input = analogRead(pin_);
  if (input < kSwitch1Threshold) return 1;
  if (input < kSwitch2Threshold) return 2;
  if (input < kSwitch3Threshold) return 3;

  return 0;
}