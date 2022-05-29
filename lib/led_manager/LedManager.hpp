#ifndef __LED_MANAGER_HPP__
#define __LED_MANAGER_HPP__

#include <Types.hpp>
#include <vector>

#include "../effect/Effect.hpp"
#include "../effect/Effects.hpp"
#include "../radio/Radio.hpp"
#include "../radio/RadioStateMachine.hpp"

class LedManager {
 public:
  LedManager(const DeviceDescription *device, RadioStateMachine *radio_state);

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
  virtual void SetLed(uint8_t led_index, CRGB *const rgb) = 0;

  // Note: these need to be defined, or else calls to this classes' constructor
  // don't work.
  virtual void WriteOutLeds() = 0;
  const DeviceDescription *const device;
  RadioStateMachine *const radio_state;

  // The effects that will be chosen randomly. This contains many entries for
  // each effect, so that we can control the occurence of each effect.
  std::vector<Effect *> effects;

  // Effects that can only be chosen manuall. These are "harsh" or otherwise
  // unsuitable for general use.
  std::vector<Effect *> non_random_effects;

  // Map of the start of each unique effect in effects.
  std::vector<uint8_t> uniqueEffectIndices;

  void AddEffect(Effect *Effect, uint8_t proportion);

  ControlEffect *control_effect;
};
#endif
