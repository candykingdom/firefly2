#ifndef __FAKE_LED_MANAGER_HPP__
#define __FAKE_LED_MANAGER_HPP__

#include <Effect.hpp>
#include <LedManager.hpp>
#include <Types.hpp>
#include <vector>

class FakeLedManager : public LedManager {
 public:
  FakeLedManager(const DeviceDescription *device,
                 RadioStateMachine *state_machine);

  CRGB GetLed(uint8_t led_index);

  void SetGlobalColor(CRGB rgb) override;

  void ClearEffects();
  void PublicAddEffect(Effect *effect, uint8_t proportion);

 protected:
  void SetLed(uint8_t led_index, CRGB *const rgb) override;

  void WriteOutLeds() override;

 private:
  // Note: this looks like a pointer, but is actually an array of size num_leds
  CRGB *leds;
};
#endif
