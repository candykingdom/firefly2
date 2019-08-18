#ifndef __FAST_LED_MANAGER_HPP__
#define __FAST_LED_MANAGER_HPP__

#include <vector>
#include "Effect.hpp"
#include "LedManager.hpp"
#include "Types.hpp"

class FastLedManager : public LedManager {
 public:
  FastLedManager(const uint8_t numLeds, DeviceType deviceType,
                 RadioStateMachine *radioState);

  void SetGlobalColor(CRGB rgb);

 protected:
  void SetLed(uint8_t ledIndex, CRGB *const rgb) override;

  void WriteOutLeds() override;

 private:
  // Note: this looks like a pointer, but is actually an array of size numLeds
  CRGB *leds;
};
#endif
