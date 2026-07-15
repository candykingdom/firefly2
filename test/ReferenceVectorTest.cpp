// Pins the firmware's rendered output against the committed reference corpus
// (sim/test/vectors/reference.json, 1,380 cases): every case is re-rendered
// through the shared VectorGenCommon case model and compared RGB-exact, so any
// unintended change to effect math, palettes, the registry, or the render
// pipeline fails CI naming the diverging case. Intentional visual changes are
// accommodated by regenerating the corpus in the same commit:
//   make vectorgen && ./vectorgen > ../sim/test/vectors/reference.json
// Schema: specs/001-web-simulator/contracts/reference-vectors.md.

#include <FastLED.h>

#include <Effect.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "VectorGenCommon.hpp"
#include "gtest/gtest.h"

namespace {

using nlohmann::json;

// Loads and parses the corpus exactly once. A missing or malformed corpus is
// a hard failure of every test in this suite (FR-002) - never a skip.
const json &Corpus(bool *ok) {
  static std::string error;
  static const json corpus = [] {
    std::ifstream in(REFERENCE_VECTORS_PATH);
    if (!in.is_open()) {
      error = "cannot open " REFERENCE_VECTORS_PATH;
      return json();
    }
    json parsed = json::parse(in, /*cb=*/nullptr, /*allow_exceptions=*/false);
    if (parsed.is_discarded()) {
      error = "cannot parse " REFERENCE_VECTORS_PATH " as JSON";
      return json();
    }
    return parsed;
  }();
  *ok = error.empty();
  EXPECT_TRUE(error.empty()) << error;
  return corpus;
}

std::string RgbToString(const CRGB &rgb) {
  std::ostringstream out;
  out << "[" << static_cast<int>(rgb.r) << "," << static_cast<int>(rgb.g)
      << "," << static_cast<int>(rgb.b) << "]";
  return out.str();
}

std::string CaseToString(const vectorgen::RenderedCase &rendered) {
  std::ostringstream out;
  if (rendered.is_control) {
    out << "control rgb " << RgbToString(rendered.control_rgb);
  } else {
    out << "effect " << static_cast<int>(rendered.effect_index) << " ('"
        << vectorgen::Effects()[rendered.effect_index].name << "'), palette "
        << static_cast<int>(rendered.palette_index);
  }
  out << ", device '" << rendered.device << "', timeMs " << rendered.time_ms;
  return out.str();
}

TEST(ReferenceVectorTest, corpusLoadsAndIsWellFormed) {
  bool ok;
  const json &corpus = Corpus(&ok);
  ASSERT_TRUE(ok);

  ASSERT_TRUE(corpus.contains("meta"));
  ASSERT_TRUE(corpus.contains("effects"));
  ASSERT_TRUE(corpus.contains("palettes"));
  ASSERT_TRUE(corpus.contains("cases"));
  ASSERT_TRUE(corpus.contains("primitives"));
  ASSERT_TRUE(corpus["cases"].is_array());
  EXPECT_GT(corpus["cases"].size(), 0u);
}

// Firefly's construction offset comes from libc rand() after srand(1), and
// rand() is implementation-specific: the corpus-recorded value (which the sim
// suite cross-checks against DEFAULT_FIREFLY_OFFSET, see docs/simulator.md)
// only reproduces on the libc family that generated the corpus. On any other
// libc the local prediction legitimately differs, and the Firefly RGB cases
// cannot be byte-compared there. Fire and Rorschach seed from FakeFastLED's
// own LCG (random16, seed 1337) and are portable.
bool FireflySeedMatchesCorpus(const json &corpus) {
  vectorgen::EffectSeeds seeds = vectorgen::PredictEffectSeedsAndResetPrngs();
  return corpus["meta"]["effectSeeds"]["Firefly"].get<uint16_t>() ==
         seeds.firefly;
}

TEST(ReferenceVectorTest, effectSeedsMatchPrediction) {
  bool ok;
  const json &corpus = Corpus(&ok);
  ASSERT_TRUE(ok);

  vectorgen::EffectSeeds seeds = vectorgen::PredictEffectSeedsAndResetPrngs();
  const json &recorded = corpus["meta"]["effectSeeds"];
  EXPECT_EQ(recorded["Fire"].get<uint16_t>(), seeds.fire);
  EXPECT_EQ(recorded["Rorschach"].get<uint16_t>(), seeds.rorschach);
  // Firefly is platform-gated (see FireflySeedMatchesCorpus). A wrong local
  // prediction is indistinguishable from a different libc here, so a
  // mismatch is reported, not failed; the corpus-recorded value itself stays
  // pinned by the sim suite's registry cross-check.
  if (recorded["Firefly"].get<uint16_t>() != seeds.firefly) {
    std::cout << "[  NOTE  ] corpus Firefly seed "
              << recorded["Firefly"].get<uint16_t>()
              << " != local libc rand() prediction " << seeds.firefly
              << " (rand() is platform-specific); Firefly cases will be "
                 "skipped in allCasesMatchFirmwareRendering\n";
  }
}

// The corpus effect table must match both the shared case model and the live
// registry: size, one representative per unique effect, and the "last two are
// DisplayColorPalette and Dark" invariant.
TEST(ReferenceVectorTest, effectTableMatchesLiveRegistry) {
  bool ok;
  const json &corpus = Corpus(&ok);
  ASSERT_TRUE(ok);

  const std::vector<vectorgen::EffectInfo> &effects = vectorgen::Effects();
  const json &table = corpus["effects"];
  ASSERT_EQ(table.size(), effects.size());
  for (size_t i = 0; i < effects.size(); i++) {
    EXPECT_EQ(table[i]["index"].get<int>(), effects[i].index);
    EXPECT_EQ(table[i]["name"].get<std::string>(), effects[i].name);
  }

  // Devices.hpp defines its catalog non-inline, so only VectorGenCommon.cpp
  // may include it; any catalog device works for registry introspection.
  vectorgen::DeviceRig rig(vectorgen::DeviceGrid()[0].description);
  // The wire table spans the random pool (GetNumEffects) followed by the
  // manually-selectable effects (GetNumNonRandomEffects).
  ASSERT_EQ(rig.manager.GetNumEffects() + rig.manager.GetNumNonRandomEffects(),
            effects.size());
  EXPECT_STREQ(effects[effects.size() - 2].name, "Display Color Palette");
  EXPECT_STREQ(effects[effects.size() - 1].name, "Dark");

  // Every unique constructed effect has exactly one representative wire index
  // in the corpus grid, so corpus coverage tracks the registry.
  const std::vector<uint8_t> &representatives =
      vectorgen::RepresentativeEffectIndices();
  ASSERT_EQ(representatives.size(), rig.manager.GetNumUniqueEffects());
  for (uint8_t u = 0; u < rig.manager.GetNumUniqueEffects(); u++) {
    EXPECT_EQ(representatives[u], rig.manager.UniqueEffectNumberToIndex(u))
        << "unique effect " << static_cast<int>(u);
  }
}

TEST(ReferenceVectorTest, paletteTableMatchesFirmware) {
  bool ok;
  const json &corpus = Corpus(&ok);
  ASSERT_TRUE(ok);

  const std::vector<vectorgen::PaletteInfo> &palettes = vectorgen::Palettes();
  const json &table = corpus["palettes"];
  ASSERT_EQ(table.size(), palettes.size());
  ASSERT_EQ(Effect::palettes().size(), palettes.size());
  for (size_t i = 0; i < palettes.size(); i++) {
    EXPECT_EQ(table[i]["index"].get<int>(), palettes[i].index);
    EXPECT_EQ(table[i]["name"].get<std::string>(), palettes[i].name);
  }
}

// The core pin: re-render every committed case through the firmware and
// compare RGB byte-exact, in corpus order.
TEST(ReferenceVectorTest, allCasesMatchFirmwareRendering) {
  bool ok;
  const json &corpus = Corpus(&ok);
  ASSERT_TRUE(ok);

  const json &cases = corpus["cases"];
  // On a libc whose rand() differs from the corpus-generating platform's,
  // Firefly renders with a different construction offset and its RGB values
  // legitimately diverge — those cases are skipped, counted, and reported
  // below (never silently). Everything else stays byte-pinned everywhere.
  const bool compare_firefly = FireflySeedMatchesCorpus(corpus);
  size_t skipped_firefly_cases = 0;
  size_t case_index = 0;
  vectorgen::ForEachCase([&cases, &case_index, compare_firefly,
                          &skipped_firefly_cases](
                             const vectorgen::RenderedCase &rendered) {
    ASSERT_LT(case_index, cases.size())
        << "firmware enumerates more cases than the corpus contains; "
           "regenerate the corpus (make vectorgen && ./vectorgen > "
           "../sim/test/vectors/reference.json)";
    const json &expected = cases[case_index];
    const std::string description =
        "case " + std::to_string(case_index) + " (" + CaseToString(rendered) +
        ")";
    // Incremented before the checks so one bad case cannot misalign the rest
    // of the enumeration (an ASSERT here only returns from this visitor).
    case_index++;

    // The descriptor must identify the same case the corpus recorded.
    if (rendered.is_control) {
      ASSERT_TRUE(expected.contains("control")) << description;
      const json &rgb = expected["control"]["rgb"];
      ASSERT_EQ(rgb[0].get<int>(), rendered.control_rgb.r) << description;
      ASSERT_EQ(rgb[1].get<int>(), rendered.control_rgb.g) << description;
      ASSERT_EQ(rgb[2].get<int>(), rendered.control_rgb.b) << description;
    } else {
      ASSERT_TRUE(expected.contains("effectIndex")) << description;
      ASSERT_EQ(expected["effectIndex"].get<int>(), rendered.effect_index)
          << description;
      ASSERT_EQ(expected["paletteIndex"].get<int>(), rendered.palette_index)
          << description;
    }
    ASSERT_EQ(expected["device"].get<std::string>(), rendered.device)
        << description;
    ASSERT_EQ(expected["timeMs"].get<uint32_t>(), rendered.time_ms)
        << description;

    const json &leds = expected["leds"];
    ASSERT_EQ(leds.size(), rendered.leds.size()) << description;

    if (!compare_firefly && !rendered.is_control &&
        std::string(vectorgen::Effects()[rendered.effect_index].name) ==
            "Firefly") {
      skipped_firefly_cases++;
      return;
    }

    for (size_t i = 0; i < rendered.leds.size(); i++) {
      const CRGB expected_rgb(leds[i][0].get<int>(), leds[i][1].get<int>(),
                              leds[i][2].get<int>());
      const CRGB &actual = rendered.leds[i];
      if (expected_rgb.r != actual.r || expected_rgb.g != actual.g ||
          expected_rgb.b != actual.b) {
        ADD_FAILURE() << description << " LED " << i << ": corpus has "
                      << RgbToString(expected_rgb) << ", firmware rendered "
                      << RgbToString(actual);
        return;  // One failure per case is enough to identify it.
      }
    }
  });
  EXPECT_EQ(case_index, cases.size())
      << "corpus contains cases the firmware no longer enumerates";

  if (!compare_firefly) {
    // Bound the skip so it can never quietly widen: exactly the Firefly
    // effect-case grid (palettes x times x devices), nothing more.
    const size_t expected_firefly_cases = vectorgen::PaletteGrid().size() *
                                          vectorgen::TimeGrid().size() *
                                          vectorgen::DeviceGrid().size();
    EXPECT_EQ(skipped_firefly_cases, expected_firefly_cases);
    std::cout << "[  NOTE  ] skipped RGB comparison for "
              << skipped_firefly_cases
              << " Firefly cases: local libc rand() does not reproduce the "
                 "corpus-generating platform's Firefly seed (see "
                 "docs/simulator.md)\n";
  }
}

// The recorded FastLED math samples must match the host FakeFastLED
// implementations the renders above depend on.
TEST(ReferenceVectorTest, primitivesMatchFakeFastLed) {
  bool ok;
  const json &corpus = Corpus(&ok);
  ASSERT_TRUE(ok);
  const json &primitives = corpus["primitives"];

  for (const json &sample : primitives["sin16"]) {
    EXPECT_EQ(sample["out"].get<int>(),
              sin16(sample["in"].get<uint16_t>()))
        << "sin16(" << sample["in"] << ")";
  }
  for (const json &sample : primitives["sin8"]) {
    EXPECT_EQ(sample["out"].get<int>(), sin8(sample["in"].get<uint8_t>()))
        << "sin8(" << sample["in"] << ")";
  }
  for (const json &sample : primitives["cubicwave8"]) {
    EXPECT_EQ(sample["out"].get<int>(),
              cubicwave8(sample["in"].get<uint8_t>()))
        << "cubicwave8(" << sample["in"] << ")";
  }
  for (const json &sample : primitives["ease8InOutApprox"]) {
    EXPECT_EQ(sample["out"].get<int>(),
              ease8InOutApprox(sample["in"].get<uint8_t>()))
        << "ease8InOutApprox(" << sample["in"] << ")";
  }
  for (const json &sample : primitives["scale8"]) {
    EXPECT_EQ(sample["out"].get<int>(),
              scale8(sample["i"].get<uint8_t>(),
                     sample["scale"].get<uint8_t>()))
        << "scale8(" << sample["i"] << "," << sample["scale"] << ")";
  }
  for (const json &sample : primitives["hsv2rgbRainbow"]) {
    const CRGB rgb = CHSV(sample["h"].get<uint8_t>(),
                          sample["s"].get<uint8_t>(),
                          sample["v"].get<uint8_t>());
    EXPECT_EQ(sample["out"][0].get<int>(), rgb.r) << "hsv " << sample;
    EXPECT_EQ(sample["out"][1].get<int>(), rgb.g) << "hsv " << sample;
    EXPECT_EQ(sample["out"][2].get<int>(), rgb.b) << "hsv " << sample;
  }

  random16_set_seed(vectorgen::kFastLedSeed);
  for (const json &sample : primitives["random16First5"]) {
    EXPECT_EQ(sample.get<uint16_t>(), random16());
  }
}

}  // namespace
