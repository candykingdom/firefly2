#ifndef __FAST_LED_MANAGER_HPP__
#define __FAST_LED_MANAGER_HPP__

#include <DeviceDescription.hpp>
#include <Effect.hpp>
#include <LedManager.hpp>
#include <Types.hpp>
#include <vector>

class FastLedManager : public LedManager {
 public:
  FastLedManager(const DeviceDescription *device,
                 RadioStateMachine *radio_state);

  void SetGlobalColor(CRGB rgb);

  void PlayStartupAnimation();

 protected:
  void SetLed(uint8_t led_index, CRGB *const rgb) override;

  void WriteOutLeds() override;

 private:
  // Note: this looks like a pointer, but is actually an array of size num_leds
  CRGB *leds;
};
#endif
