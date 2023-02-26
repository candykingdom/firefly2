#include "LedManager.hpp"

#include <Effects.hpp>
#include <Radio.hpp>
#include <cassert>

LedManager::LedManager(const DeviceDescription *device,
                       RadioStateMachine *radio_state)
    : device(device), radio_state(radio_state) {
  AddEffect(new ColorCycleEffect(), 2);
  AddEffect(new ContrastBumpsEffect(), 2);
  AddEffect(new FireEffect(), 1);
  AddEffect(new FireflyEffect(), 2);
  AddEffect(new LightningEffect(), 1);
  AddEffect(new PrideEffect(), 1);
  AddEffect(new RainbowBumpsEffect(), 4);
  AddEffect(new RainbowEffect(), 4);
  AddEffect(new RorschachEffect(), 2);
  AddEffect(new SparkEffect(), 4);
  AddEffect(new SwingingLights(device), 4);

  // Non-random effects
  AddEffect(new PoliceEffect(), 0);
  AddEffect(new StopLightEffect(), 0);
  // Strobes
  AddEffect(new SimpleBlinkEffect(60), 0);
  AddEffect(new SimpleBlinkEffect(30), 0);
  AddEffect(new SimpleBlinkEffect(12), 0);
  AddEffect(new SimpleBlinkEffect(300), 0);

  // These two must be last
  AddEffect(new DisplayColorPaletteEffect(), 0);
  AddEffect(new DarkEffect(), 0);

  control_effect = new ControlEffect();

  radio_state->SetNumEffects(GetNumEffects());
  radio_state->SetNumPalettes(Effect::palettes().size());
}

LedManager::~LedManager() {
  for (uint8_t effect_index : uniqueEffectIndices) {
    delete effects[effect_index];
  }
  for (Effect *effect : non_random_effects) {
    delete effect;
  }
  delete control_effect;
}

Effect *LedManager::GetCurrentEffect() {
  RadioPacket *packet = radio_state->GetSetEffect();
  if (packet->type == SET_CONTROL) {
    return control_effect;
  }
  uint8_t effect_index =
      radio_state->GetSetEffect()->readEffectIndexFromSetEffect();
  return GetEffect(effect_index);
}

Effect *LedManager::GetEffect(uint8_t index) {
  uint8_t total_num_effects = effects.size() + non_random_effects.size();
#ifdef ARDUINO
  index = index % total_num_effects;
#else
  assert(index < total_num_effects);
#endif

  if (index < effects.size()) {
    return effects[index];
  } else {
    return non_random_effects[index - effects.size()];
  }
}

void LedManager::RunEffect() {
  uint8_t global_index = 0;
  for (auto it = device->strips.begin(); it != device->strips.end(); ++it) {
    const StripDescription *strip = *it;
    for (uint8_t strip_index = 0; strip_index < strip->led_count;
         ++strip_index) {
      uint8_t virtual_index;
      if (strip->FlagEnabled(Reversed)) {
        virtual_index = strip->led_count - strip_index - 1;
      } else {
        virtual_index = strip_index;
      }

      CRGB rgb = GetCurrentEffect()->GetRGB(virtual_index,
                                            radio_state->GetNetworkMillis(),
                                            strip, radio_state->GetSetEffect());
      SetLed(global_index, rgb);
      global_index += 1;
    }
  }
  WriteOutLeds();
}

uint8_t LedManager::GetNumEffects() { return effects.size(); }

uint8_t LedManager::GetNumUniqueEffects() {
  return uniqueEffectIndices.size() + non_random_effects.size();
}

uint8_t LedManager::GetNumNonRandomEffects() {
  return non_random_effects.size();
}

uint8_t LedManager::UniqueEffectNumberToIndex(uint8_t uniqueEffectNumber) {
  if (uniqueEffectNumber < uniqueEffectIndices.size()) {
    return uniqueEffectIndices[uniqueEffectNumber];
  } else {
    return effects.size() + (uniqueEffectNumber - uniqueEffectIndices.size());
  }
}

void LedManager::AddEffect(Effect *effect, uint8_t proportion) {
  if (proportion > 0) {
    uniqueEffectIndices.push_back(effects.size());
  } else {
    non_random_effects.push_back(effect);
  }
  for (uint8_t i = 0; i < proportion; ++i) {
    effects.push_back(effect);
  }
#ifndef ARDUINO
  // The effect index is 8 bits, so make sure the total number of effects is in
  // range.
  assert(effects.size() + non_random_effects.size() < 256);
#endif
}
