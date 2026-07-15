#include <Arduino.h>
#include <edge-filter.h>

#include "Types.hpp"

class AnalogButton {
 public:
  explicit AnalogButton(int pin);
  void Tick();

  bool Button1Pressed() const;
  bool Button2Pressed() const;
  bool Button3Pressed() const;

 private:
  int ReadAnalog() const;

  EdgeFilter analog_filter_;

  const int pin_;
  int prev_state_ = 0;

  bool button1_pressed_ = false;
  bool button2_pressed_ = false;
  bool button3_pressed_ = false;

  // Constants for the analog switch inputs. See
  // http://www.ignorantofthings.com/2018/07/the-perfect-multi-button-input-resistor.html?m=1
  static constexpr uint16_t kSwitch1Threshold = 346 / 2;
  static constexpr uint16_t kSwitch2Threshold = (346 + 678) / 2;
  static constexpr uint16_t kSwitch3Threshold = (678 + 1023) / 2;

  // The slope of the input must be below this value before we consider it
  // stable. This is needed because the filter cap on the input slows the edge
  // transition. If we don't wait until the input is somewhat stable, we'll read
  // pressing button 1 as button 2 instead, since the input transition from VCC
  // to GND when button 1 is pressed. Larger values mean a faster response, but
  // higher chance of incorrectly triggering.
  static constexpr int32_t kStableThreshold = 100;
  // The alpha value for the exponential moving average filter for edge
  // detection. Making (255 - kFilterAlpha) a power of 2 makes this faster and
  // less prone to rounding issues.
  static constexpr uint8_t kFilterAlpha = 251;
};