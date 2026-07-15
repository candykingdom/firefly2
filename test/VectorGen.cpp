// Renders firmware effects through the host-test fakes and prints reference
// vectors as JSON to stdout. This is the fidelity ground truth used by the
// JavaScript simulator port (specs/001-web-simulator) and pinned by
// test/ReferenceVectorTest.cpp. See
// specs/001-web-simulator/contracts/reference-vectors.md for the schema.
//
// The case model (seeds, wire tables, device catalog, case enumeration) lives
// in VectorGenCommon.{hpp,cpp}, shared with ReferenceVectorTest so generator
// and test can never drift apart. This file only emits JSON.
//
// Usage:
//   mkdir -p build && cd build
//   cmake .. -DBUILD_SIMULATOR=false && make vectorgen
//   ./vectorgen > ../sim/test/vectors/reference.json
//
// This binary is intentionally not part of smalltests/largetests.

#include <FastLED.h>

#include <Types.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "VectorGenCommon.hpp"

namespace {

std::string GitDescribe() {
  FILE *pipe = popen("git describe --always --dirty 2>/dev/null", "r");
  if (pipe == nullptr) {
    return "unknown";
  }
  std::string result;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  int status = pclose(pipe);
  if (status != 0 || result.empty()) {
    return "unknown";
  }
  while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
    result.pop_back();
  }
  return result.empty() ? "unknown" : result;
}

// Minimal, deterministic hand-rolled JSON emitter: integers only, fixed key
// order, no third-party dependencies.

void WriteJsonString(std::ostream &out, const std::string &value) {
  out << '"';
  for (char c : value) {
    if (c == '"' || c == '\\') {
      out << '\\';
    }
    out << c;
  }
  out << '"';
}

void WriteRgb(std::ostream &out, const CRGB &rgb) {
  out << "[" << static_cast<int>(rgb.r) << "," << static_cast<int>(rgb.g) << ","
      << static_cast<int>(rgb.b) << "]";
}

void WriteCase(std::ostream &out, const vectorgen::RenderedCase &rendered,
               bool *first) {
  if (!*first) {
    out << ",";
  }
  *first = false;
  if (rendered.is_control) {
    out << "\n    {\"control\":{\"rgb\":";
    WriteRgb(out, rendered.control_rgb);
    out << "},\"device\":";
    WriteJsonString(out, rendered.device);
  } else {
    out << "\n    {\"effectIndex\":" << static_cast<int>(rendered.effect_index)
        << ",\"paletteIndex\":" << static_cast<int>(rendered.palette_index)
        << ",\"device\":";
    WriteJsonString(out, rendered.device);
  }
  out << ",\"timeMs\":" << rendered.time_ms << ",\"leds\":[";
  for (size_t i = 0; i < rendered.leds.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    WriteRgb(out, rendered.leds[i]);
  }
  out << "]}";
}

void WritePrimitives(std::ostream &out) {
  out << "\"primitives\":{\n";

  out << "    \"sin16\":[";
  const std::vector<uint16_t> sin16_inputs = {0,     8192,  16384,
                                              32768, 49152, 65535};
  for (size_t i = 0; i < sin16_inputs.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"in\":" << sin16_inputs[i]
        << ",\"out\":" << static_cast<int>(sin16(sin16_inputs[i])) << "}";
  }
  out << "],\n";

  out << "    \"sin8\":[";
  const std::vector<uint8_t> sin8_inputs = {0, 64, 128, 192, 255};
  for (size_t i = 0; i < sin8_inputs.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"in\":" << static_cast<int>(sin8_inputs[i])
        << ",\"out\":" << static_cast<int>(sin8(sin8_inputs[i])) << "}";
  }
  out << "],\n";

  out << "    \"cubicwave8\":[";
  const std::vector<uint8_t> cubicwave8_inputs = {0, 32, 64, 128, 196};
  for (size_t i = 0; i < cubicwave8_inputs.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"in\":" << static_cast<int>(cubicwave8_inputs[i])
        << ",\"out\":" << static_cast<int>(cubicwave8(cubicwave8_inputs[i]))
        << "}";
  }
  out << "],\n";

  out << "    \"ease8InOutApprox\":[";
  const std::vector<uint8_t> ease8_inputs = {0, 63, 64, 128, 191, 192, 255};
  for (size_t i = 0; i < ease8_inputs.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"in\":" << static_cast<int>(ease8_inputs[i])
        << ",\"out\":" << static_cast<int>(ease8InOutApprox(ease8_inputs[i]))
        << "}";
  }
  out << "],\n";

  out << "    \"scale8\":[";
  const std::vector<std::pair<uint8_t, uint8_t>> scale8_inputs = {
      {128, 128}, {255, 255}, {1, 255}, {255, 1}};
  for (size_t i = 0; i < scale8_inputs.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"i\":" << static_cast<int>(scale8_inputs[i].first)
        << ",\"scale\":" << static_cast<int>(scale8_inputs[i].second)
        << ",\"out\":"
        << static_cast<int>(
               scale8(scale8_inputs[i].first, scale8_inputs[i].second))
        << "}";
  }
  out << "],\n";

  out << "    \"hsv2rgbRainbow\":[";
  const std::vector<CHSV> hsv_inputs = {
      CHSV(0, 255, 255),   CHSV(32, 255, 255),  CHSV(96, 255, 255),
      CHSV(160, 255, 255), CHSV(224, 255, 255), CHSV(0, 0, 200),
      CHSV(33, 241, 249),  CHSV(128, 128, 128),
  };
  for (size_t i = 0; i < hsv_inputs.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    const CHSV &hsv = hsv_inputs[i];
    CRGB rgb = hsv;
    out << "{\"h\":" << static_cast<int>(hsv.h)
        << ",\"s\":" << static_cast<int>(hsv.s)
        << ",\"v\":" << static_cast<int>(hsv.v) << ",\"out\":";
    WriteRgb(out, rgb);
    out << "}";
  }
  out << "],\n";

  // RESEED before sampling the raw LCG stream, done last so it doesn't
  // disturb construction/render seeds above.
  random16_set_seed(vectorgen::kFastLedSeed);
  out << "    \"random16First5\":[";
  for (int i = 0; i < 5; i++) {
    if (i > 0) {
      out << ",";
    }
    out << random16();
  }
  out << "]\n";

  out << "  }";
}

}  // namespace

int main() {
  vectorgen::EffectSeeds seeds = vectorgen::PredictEffectSeedsAndResetPrngs();

  std::ostringstream out;
  out << "{\n";

  out << "  \"meta\":{\n";
  out << "    \"generator\":";
  WriteJsonString(out, "test/VectorGen.cpp");
  out << ",\n    \"firmwareGitDescribe\":";
  WriteJsonString(out, GitDescribe());
  out << ",\n    \"effectSeeds\":{\"Fire\":" << seeds.fire
      << ",\"Firefly\":" << seeds.firefly
      << ",\"Rorschach\":" << seeds.rorschach << "}\n";
  out << "  },\n";

  out << "  \"effects\":[";
  const std::vector<vectorgen::EffectInfo> &effects = vectorgen::Effects();
  for (size_t i = 0; i < effects.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"index\":" << static_cast<int>(effects[i].index) << ",\"name\":";
    WriteJsonString(out, effects[i].name);
    out << "}";
  }
  out << "],\n";

  out << "  \"palettes\":[";
  const std::vector<vectorgen::PaletteInfo> &palettes = vectorgen::Palettes();
  for (size_t i = 0; i < palettes.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"index\":" << static_cast<int>(palettes[i].index) << ",\"name\":";
    WriteJsonString(out, palettes[i].name);
    out << "}";
  }
  out << "],\n";

  out << "  \"cases\":[";
  bool first_case = true;
  vectorgen::ForEachCase([&out, &first_case](
                             const vectorgen::RenderedCase &rendered) {
    WriteCase(out, rendered, &first_case);
  });
  out << "\n  ],\n";

  out << "  ";
  WritePrimitives(out);
  out << "\n}\n";

  std::cout << out.str();
  return 0;
}
