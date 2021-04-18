#ifndef __LED_MANAGER_HPP__
#define __LED_MANAGER_HPP__

#include <vector>

#include "Effect.hpp"
#include "Radio.hpp"
#include "RadioStateMachine.hpp"
#include "ControlEffect.hpp"
#include "Types.hpp"

class LedManager {
 public:
  LedManager(const uint8_t numLeds, DeviceType deviceType,
             RadioStateMachine *radioState);

  void RunEffect();

  Effect *GetCurrentEffect();

  /**
   * Returns the total number of effects (including the many copies in effects
   * and the non-random effects).
   */
  uint8_t GetNumEffects();

  uint8_t GetNumUniqueEffects();

  uint8_t GetNumNonRandomEffects();

  /**
   * Converts from a unique effect index (between 0 and GetNumUniqueEffects())
   * to the effect index used for SetEffect packets.
   */
  uint8_t UniqueEffectNumberToIndex(uint8_t uniqueEffectNumber);

  Effect *GetEffect(uint8_t index);

  /** Sets all LEDs to the given color. */
  virtual void SetGlobalColor(CRGB rgb) = 0;

 protected:
  virtual void SetLed(uint8_t ledIndex, CRGB *const rgb) = 0;

  // Note: these need to be defined, or else calls to this classes' constructor
  // don't work.
  virtual void WriteOutLeds() = 0;
  const uint8_t numLeds;
  RadioStateMachine *const radioState;

  // The effects that will be chosen randomly. This contains many entries for
  // each effect, so that we can control the occurence of each effect.
  std::vector<Effect *> effects;

  // Effects that can only be chosen manuall. These are "harsh" or otherwise
  // unsuitable for general use.
  std::vector<Effect *> nonRandomEffects;

  // Map of the start of each unique effect in effects.
  std::vector<uint8_t> uniqueEffectIndices;

  void AddEffect(Effect *Effect, uint8_t proportion);

  ControlEffect* controlEffect;
};
#endif
