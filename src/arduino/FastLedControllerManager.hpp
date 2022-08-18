#ifndef __FAST_LED_CONTROLLER_MANAGER_HPP__
#define __FAST_LED_CONTROLLER_MANAGER_HPP__

#include <Effect.hpp>
#include <LedManager.hpp>
#include <Types.hpp>
#include <vector>

#include "../../lib/effect/Effect.hpp"

const uint8_t kControllerLedCount = 42;
const uint8_t kControllerLightsPerRow = 12;

class FastLedControllerManager : public LedManager {
 public:
  FastLedControllerManager(RadioStateMachine* radio_state);

  enum Row {
    TOP_ROW,
    MIDDLE_ROW,
    BOTTOM_ROW,
  };

  enum Side {
    LEFT_SIDE,
    MIDDLE_SIDE,
    RIGHT_SIDE,
  };

  void RunEffect();

  void SetGlobalColor(CRGB rgb);

 protected:
  void SetLed(uint8_t led_index, CRGB* const rgb);
  void WriteOutLeds();

 private:
  // Note: this looks like a pointer, but is actually an array of size num_leds
  CRGB* leds;

  uint8_t effect_index = 0;
  uint8_t palette_index = 0;
};
#endif  // __FAST_LED_CONTROLLER_MANAGER_HPP__
