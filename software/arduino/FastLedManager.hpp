#ifndef __FAST_LED_MANAGER_HPP__
#define __FAST_LED_MANAGER_HPP__

#include <Types.hpp>
#include <vector>

#include "../generic/Effect.hpp"
#include "../generic/LedManager.hpp"

class FastLedManager : public LedManager {
 public:
  FastLedManager(const DeviceDescription *device,
                 RadioStateMachine *radioState);

  void SetGlobalColor(CRGB rgb);

  void PlayStartupAnimation();

 protected:
  void SetLed(uint8_t ledIndex, CRGB *const rgb) override;

  void WriteOutLeds() override;

 private:
  // Note: this looks like a pointer, but is actually an array of size numLeds
  CRGB *leds;
};
#endif
