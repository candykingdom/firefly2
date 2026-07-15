// Shared case model for the reference-vector corpus: seed prediction, the
// effect/palette wire tables, the device catalog, and the case enumeration
// that renders every corpus case in committed order. Consumed by both the
// generator (test/VectorGen.cpp, emits JSON) and the corpus regression test
// (test/ReferenceVectorTest.cpp, compares), so the two can never drift apart.
// Schema: specs/001-web-simulator/contracts/reference-vectors.md.
//
// Deliberately named so the CMake glob exclusion `.*VectorGen\.cpp$` does NOT
// match VectorGenCommon.cpp, which therefore lands in testlib.

#ifndef TEST_VECTOR_GEN_COMMON_HPP_
#define TEST_VECTOR_GEN_COMMON_HPP_

#include <FastLED.h>

#include <DeviceDescription.hpp>
#include <FakeLedManager.hpp>
#include <FakeRadio.hpp>
#include <NetworkManager.hpp>
#include <RadioStateMachine.hpp>
#include <Types.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace vectorgen {

// The FastLED LCG seed used by FireEffect/RorschachEffect construction (host
// builds skip the ARDUINO-only re-seed from analogRead), and by the
// `primitives.random16First5` sample.
constexpr uint16_t kFastLedSeed = 1337;

struct EffectSeeds {
  uint16_t fire;
  uint16_t rorschach;
  uint16_t firefly;
};

// Predicts the constructor-time random offsets consumed by FireEffect,
// FireflyEffect and RorschachEffect (the only effects that consume randomness
// at construction time), then resets both PRNGs so a subsequent LedManager
// construction reproduces the exact same sequence.
//
// Must be called immediately before constructing the LedManager: the
// RadioStateMachine constructor's beginSlave() consumes one libc rand() for
// its timer jitter, which would otherwise shift FireflyEffect's draw.
EffectSeeds PredictEffectSeedsAndResetPrngs();

struct EffectInfo {
  uint8_t index;
  const char *name;
};

// The full computed wire table (LedManager.cpp): weights 2,2,1,2,1,1,4,4,2,4,4
// for the random effects (indices 0-26), then 8 non-random effects (indices
// 27-34, with 33=DisplayColorPalette and 34=Dark last per the registry
// invariant).
const std::vector<EffectInfo> &Effects();

// One representative wire index per unique constructed effect object.
const std::vector<uint8_t> &RepresentativeEffectIndices();

struct PaletteInfo {
  uint8_t index;
  const char *name;
};

const std::vector<PaletteInfo> &Palettes();

// The palette indices sampled by the corpus cases.
const std::vector<uint8_t> &PaletteGrid();

// The network timestamps sampled by the corpus cases.
const std::vector<uint32_t> &TimeGrid();

struct DeviceInfo {
  const char *name;
  const DeviceDescription &description;
};

const std::vector<DeviceInfo> &DeviceGrid();

// The solid colors and timestamps sampled by the control-override cases.
const std::vector<CRGB> &ControlRgbGrid();
const std::vector<uint32_t> &ControlTimeGrid();

// Owns the fakes needed to render a device's effects, resetting the PRNGs
// between the RadioStateMachine member (whose constructor consumes a libc
// rand() for timer jitter) and the LedManager member (whose effect
// construction must consume the predicted sequence).
struct DeviceRig {
  explicit DeviceRig(const DeviceDescription &device);

  FakeRadio radio;
  NetworkManager network_manager;
  RadioStateMachine state_machine;
  FakeLedManager manager;
};

// One rendered corpus case: the descriptor identifying it plus the LED output.
struct RenderedCase {
  bool is_control;
  const char *device;
  uint8_t effect_index;   // effect cases only
  uint8_t palette_index;  // effect cases only
  CRGB control_rgb;       // control cases only
  uint32_t time_ms;
  std::vector<CRGB> leds;
};

// Enumerates every corpus case in committed-corpus order (all effect cases
// across the device grid, then all control cases), rendering each through
// per-device rigs exactly as the committed corpus was generated.
void ForEachCase(const std::function<void(const RenderedCase &)> &visit);

}  // namespace vectorgen

#endif  // TEST_VECTOR_GEN_COMMON_HPP_
