#include "LedManager.hpp"

#include <Radio.hpp>
#include <cassert>

LedManager::LedManager(const DeviceDescription *device,
                       RadioStateMachine *radio_state)
    : device(device), radio_state(radio_state) {
  AddEffect(new ColorCycleEffect(device), 4);
  AddEffect(new ContrastBumpsEffect(device), 2);
  AddEffect(new FireEffect(device), 1);
  AddEffect(new FireflyEffect(device), 2);
  AddEffect(new LightningEffect(device), 1);
  AddEffect(new RainbowBumpsEffect(device), 4);
  AddEffect(new RainbowEffect(device), 4);
  AddEffect(new RorschachEffect(device), 2);
  AddEffect(new SimpleBlinkEffect(device, 300), 2);
  AddEffect(new SparkEffect(device), 4);
  AddEffect(new SwingingLights(device), 4);

  // Non-random effects
  AddEffect(new PoliceEffect(device), 0);
  AddEffect(new StopLightEffect(device), 0);
  // Strobes
  AddEffect(new SimpleBlinkEffect(device, 60), 0);
  AddEffect(new SimpleBlinkEffect(device, 30), 0);
  AddEffect(new SimpleBlinkEffect(device, 12), 0);

  // These two must be last
  AddEffect(new DisplayColorPaletteEffect(device), 0);
  AddEffect(new DarkEffect(device), 0);

  control_effect = new ControlEffect(device);

  radio_state->SetNumEffects(GetNumEffects());
  radio_state->SetNumPalettes(effects[0]->palettes.size());
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
  for (uint8_t led_index = 0; led_index < device->led_count; led_index++) {
    CRGB rgb =
        GetCurrentEffect()->GetRGB(led_index, radio_state->GetNetworkMillis(),
                                   radio_state->GetSetEffect());
    SetLed(led_index, &rgb);
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
