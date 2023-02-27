#include <Arduino.h>
#include <median-filter.h>

#include "Types.hpp"

class AnalogButton {
 public:
  AnalogButton(int pin);
  void Tick();

  bool Button1Pressed();
  bool Button2Pressed();
  bool Button3Pressed();

 private:
  int ReadAnalog();

  MedianFilter<uint16_t, uint16_t, 3> analog_filter_;

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
};