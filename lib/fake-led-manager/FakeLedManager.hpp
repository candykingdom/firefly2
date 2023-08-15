#ifndef __FAKE_LED_MANAGER_HPP__
#define __FAKE_LED_MANAGER_HPP__

#include <LedManager.hpp>
#include <Types.hpp>
#include <vector>

#include "Effect.hpp"

class FakeLedManager : public LedManager {
 public:
  explicit FakeLedManager(const DeviceDescription &device,
                          RadioStateMachine *state_machine);
  ~FakeLedManager();

  CRGB GetLed(uint16_t led_index);

  void SetGlobalColor(const CRGB &rgb) override;

  void ClearEffects();
  void PublicAddEffect(Effect *effect, uint8_t proportion);

 protected:
  void SetLed(uint16_t led_index, const CRGB &rgb) override;

  void WriteOutLeds() override;

 private:
  uint16_t led_count;

  // Note: this looks like a pointer, but is actually an array of size num_leds
  CRGB *leds;
};
#endif
