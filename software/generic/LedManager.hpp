#ifndef __LED_MANAGER_HPP__
#define __LED_MANAGER_HPP__

#include <vector>
#include "Effect.hpp"
#include "Types.hpp"

class LedManager {
 public:
  LedManager(const uint8_t numLeds);

  void RunEffect(uint32_t timeMillis);

  void SetEffect(uint8_t effectIndex);

 protected:
  virtual void SetLed(uint8_t ledIndex, CRGB &rgb) = 0;

  // Note: these need to be defined, or else calls to this classes' constructor
  // don't work.
  virtual void WriteOutLeds() = 0;
  const uint8_t numLeds;

 private:
  std::vector<Effect> effects;
  uint8_t effectIndex = 0;
};
#endif
