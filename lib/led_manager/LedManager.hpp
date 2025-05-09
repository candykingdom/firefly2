#ifndef __LED_MANAGER_HPP__
#define __LED_MANAGER_HPP__

#include <Effect.hpp>
#include <Effects.hpp>
#include <Radio.hpp>
#include <Types.hpp>
#include <vector>

#include "../../src/generic/RadioStateMachine.hpp"

class LedManager {
 public:
  explicit LedManager(const DeviceDescription &device,
                      RadioStateMachine *radio_state);
  virtual ~LedManager();

  void RunEffect();

  Effect *GetCurrentEffect();

  /**
   * Returns the total number of effects (including the many copies in effects
   * and the non-random effects).
   */
  uint8_t GetNumEffects() const;

  uint8_t GetNumUniqueEffects() const;

  uint8_t GetNumNonRandomEffects() const;

  /**
   * Converts from a unique effect index (between 0 and GetNumUniqueEffects())
   * to the effect index used for SetEffect packets.
   */
  uint8_t UniqueEffectNumberToIndex(uint8_t uniqueEffectNumber) const;

  Effect *GetEffect(uint8_t index);

  /** Sets all LEDs to the given color. */
  virtual void SetGlobalColor(const CRGB &rgb) = 0;

  virtual void SetLed(uint8_t led_index, const CRGB &rgb) = 0;

 protected:
  // Note: these need to be defined, or else calls to this classes' constructor
  // don't work.
  virtual void WriteOutLeds() = 0;
  const DeviceDescription &device;
  RadioStateMachine *const radio_state;

  // The effects that will be chosen randomly. This contains many entries for
  // each effect, so that we can control the occurence of each effect.
  std::vector<Effect *> effects;

  // Effects that can only be chosen manually. These are "harsh" or otherwise
  // unsuitable for general use.
  std::vector<Effect *> non_random_effects;

  // Map of the start of each unique effect in effects.
  std::vector<uint8_t> uniqueEffectIndices;

  void AddEffect(Effect *Effect, uint8_t proportion);

  ControlEffect *const control_effect;
};
#endif
