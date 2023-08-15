#ifndef __FAST_LED_MANAGER_HPP__
#define __FAST_LED_MANAGER_HPP__

#include <DeviceDescription.hpp>
#include <LedManager.hpp>
#include <Types.hpp>
#include <vector>

#include "../../lib/effect/Effect.hpp"

class FastLedManager : public LedManager {
 public:
  explicit FastLedManager(const DeviceDescription &device,
                          RadioStateMachine *radio_state);

  void SetGlobalColor(const CRGB &rgb);

  void PlayStartupAnimation();

  // Blinks the LEDs red. Never returns; call upon unrecoverable error.
  void FatalErrorAnimation();

  void SetLed(uint16_t led_index, const CRGB &rgb) override;

  void SetOnboardLed(const CRGB &rgb);

 protected:
  void WriteOutLeds() override;

 private:
  // Note: this looks like a pointer, but is actually an array of size num_leds
  CRGB *leds;
};
#endif
