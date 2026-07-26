#ifndef LIB_EFFECT_EFFECT_REGISTRY_HPP_
#define LIB_EFFECT_EFFECT_REGISTRY_HPP_

#include <Effect.hpp>
#include <cstdint>
#include <vector>

struct EffectSeedOverrides {
  uint16_t fire_offset;
  uint32_t firefly_offset;
  uint16_t rorschach_offset;
};

enum class EffectKind : uint8_t {
  ColorCycle,
  ContrastBumps,
  Fire,
  Firefly,
  Lightning,
  MitxelaPiercing,
  Pride,
  RainbowBumps,
  Rainbow,
  Rorschach,
  Spark,
  SwingingLights,
  StopLight,
  SimpleBlink,
  DisplayColorPalette,
  Dark,
};

struct EffectDeclaration {
  const char *name;
  uint8_t weight;
  uint16_t parameter;
  EffectKind kind;
};

namespace EffectRegistry {

const std::vector<EffectDeclaration> &Declarations();

// The expanded wire-index table. Weighted declarations appear once per unit
// of weight; manual-only declarations (weight zero) appear once in the tail.
const std::vector<const EffectDeclaration *> &WireTable();

uint8_t RandomEffectCount();

// The caller owns the returned effect. A null seed pointer preserves the
// production constructors and their hardware-random offset behavior.
Effect *CreateEffect(const EffectDeclaration &declaration,
                     const EffectSeedOverrides *seeds = nullptr);

}  // namespace EffectRegistry

#endif  // LIB_EFFECT_EFFECT_REGISTRY_HPP_
