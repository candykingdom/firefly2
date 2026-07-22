#include "EffectRegistry.hpp"

#include <Effects.hpp>
#include <cassert>

namespace EffectRegistry {

const std::vector<EffectDeclaration>& Declarations() {
  static const std::vector<EffectDeclaration> declarations = {
      {"Color Cycle", 2, 0, EffectKind::ColorCycle},
      {"Contrast Bumps", 2, 0, EffectKind::ContrastBumps},
      {"Fire", 1, 0, EffectKind::Fire},
      {"Firefly", 2, 0, EffectKind::Firefly},
      {"Lightning", 1, 0, EffectKind::Lightning},
      {"Pride", 1, 0, EffectKind::Pride},
      {"Rainbow Bumps", 4, 0, EffectKind::RainbowBumps},
      {"Rainbow", 4, 0, EffectKind::Rainbow},
      {"Rorschach", 2, 0, EffectKind::Rorschach},
      {"Spark", 4, 0, EffectKind::Spark},
      {"Swinging Lights", 4, 0, EffectKind::SwingingLights},
      {"Swinging Lights (Police)", 0, 0, EffectKind::SwingingLights},
      {"Stop Light", 0, 0, EffectKind::StopLight},
      {"Simple Blink 60ms", 0, 60, EffectKind::SimpleBlink},
      {"Simple Blink 30ms", 0, 30, EffectKind::SimpleBlink},
      {"Simple Blink 12ms", 0, 12, EffectKind::SimpleBlink},
      {"Simple Blink 300ms", 0, 300, EffectKind::SimpleBlink},
      {"Display Color Palette", 0, 0, EffectKind::DisplayColorPalette},
      {"Dark", 0, 0, EffectKind::Dark},
  };
  return declarations;
}

const std::vector<const EffectDeclaration*>& WireTable() {
  static const std::vector<const EffectDeclaration*> wire_table = [] {
    std::vector<const EffectDeclaration*> result;
    for (const EffectDeclaration& declaration : Declarations()) {
      for (uint8_t copy = 0; copy < declaration.weight; ++copy) {
        result.push_back(&declaration);
      }
    }
    for (const EffectDeclaration& declaration : Declarations()) {
      if (declaration.weight == 0) {
        result.push_back(&declaration);
      }
    }
    assert(result.size() < 256);
    assert(result.size() >= 2);
    assert(result[result.size() - 2]->kind == EffectKind::DisplayColorPalette);
    assert(result.back()->kind == EffectKind::Dark);
    return result;
  }();
  return wire_table;
}

uint8_t RandomEffectCount() {
  uint16_t count = 0;
  for (const EffectDeclaration& declaration : Declarations()) {
    count += declaration.weight;
  }
  assert(count < 256);
  return static_cast<uint8_t>(count);
}

Effect* CreateEffect(const EffectDeclaration& declaration,
                     const EffectSeedOverrides* seeds) {
  switch (declaration.kind) {
    case EffectKind::ColorCycle:
      return new ColorCycleEffect();
    case EffectKind::ContrastBumps:
      return new ContrastBumpsEffect();
    case EffectKind::Fire:
      return seeds == nullptr ? static_cast<Effect*>(new FireEffect())
                              : new FireEffect(seeds->fire_offset);
    case EffectKind::Firefly:
      return seeds == nullptr ? static_cast<Effect*>(new FireflyEffect())
                              : new FireflyEffect(seeds->firefly_offset);
    case EffectKind::Lightning:
      return new LightningEffect();
    case EffectKind::Pride:
      return new PrideEffect();
    case EffectKind::RainbowBumps:
      return new RainbowBumpsEffect();
    case EffectKind::Rainbow:
      return new RainbowEffect();
    case EffectKind::Rorschach:
      return seeds == nullptr ? static_cast<Effect*>(new RorschachEffect())
                              : new RorschachEffect(seeds->rorschach_offset);
    case EffectKind::Spark:
      return new SparkEffect();
    case EffectKind::SwingingLights:
      return new SwingingLights();
    case EffectKind::StopLight:
      return new StopLightEffect();
    case EffectKind::SimpleBlink:
      return new SimpleBlinkEffect(declaration.parameter);
    case EffectKind::DisplayColorPalette:
      return new DisplayColorPaletteEffect();
    case EffectKind::Dark:
      return new DarkEffect();
  }
  return nullptr;
}

}  // namespace EffectRegistry
