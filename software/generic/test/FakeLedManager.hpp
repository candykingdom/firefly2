#ifndef __FAKE_LED_MANAGER_HPP__
#define __FAKE_LED_MANAGER_HPP__

#include <vector>
#include "Effect.hpp"
#include "LedManager.hpp"
#include "Types.hpp"

class FakeLedManager : public LedManager {
 public:
  FakeLedManager(const uint8_t numLeds);

 protected:
  void SetLed(uint8_t ledIndex, CRGB &rgb) override;

  void WriteOutLeds() override;

 private:
  // Note: this looks like a pointer, but is actually an array of size numLeds
  CRGB *leds;
};
#endif
