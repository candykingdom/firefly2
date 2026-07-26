#include <DeviceCatalog.hpp>
#include <EffectRegistry.hpp>

#include <FireEffect.hpp>
#include <FireflyEffect.hpp>
#include <RorschachEffect.hpp>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

uint32_t PackHsv(const CHSV &color) {
  return static_cast<uint32_t>(color.h) |
         (static_cast<uint32_t>(color.s) << 8) |
         (static_cast<uint32_t>(color.v) << 16);
}

void ExpectSameRendering(const Effect &actual, const Effect &expected) {
  const StripDescription strip(17, {Circular, Mirrored});
  RadioPacket packet;
  packet.writeSetEffect(0, 0, 8);
  for (uint8_t led : {static_cast<uint8_t>(0), static_cast<uint8_t>(8),
                      static_cast<uint8_t>(16)}) {
    for (uint32_t time : {0u, 1u, 1000u, 60000u, 0xffffffffu}) {
      const CRGB actual_rgb = actual.GetRGB(led, time, strip, &packet);
      const CRGB expected_rgb = expected.GetRGB(led, time, strip, &packet);
      EXPECT_EQ(actual_rgb.r, expected_rgb.r);
      EXPECT_EQ(actual_rgb.g, expected_rgb.g);
      EXPECT_EQ(actual_rgb.b, expected_rgb.b);
    }
  }
}

TEST(SimulatorCatalogTest, EffectRegistryPreservesOrderWeightsAndWireTable) {
  const struct {
    const char *name;
    uint8_t weight;
    uint16_t parameter;
  } expected[] = {
      {"Color Cycle", 2, 0},
      {"Contrast Bumps", 2, 0},
      {"Fire", 1, 0},
      {"Firefly", 2, 0},
      {"Lightning", 1, 0},
      {"Pride", 1, 0},
      {"Rainbow Bumps", 4, 0},
      {"Rainbow", 4, 0},
      {"Rorschach", 2, 0},
      {"Spark", 4, 0},
      {"Swinging Lights", 4, 0},
      {"Swinging Lights (Police)", 0, 0},
      {"Stop Light", 0, 0},
      {"Simple Blink 60ms", 0, 60},
      {"Simple Blink 30ms", 0, 30},
      {"Simple Blink 12ms", 0, 12},
      {"Simple Blink 300ms", 0, 300},
      {"Mitxela Piercing", 0, 0},
      {"Display Color Palette", 0, 0},
      {"Dark", 0, 0},
  };

  const auto &declarations = EffectRegistry::Declarations();
  ASSERT_EQ(declarations.size(),
            sizeof(expected) / sizeof(expected[0]));
  for (size_t i = 0; i < declarations.size(); ++i) {
    EXPECT_STREQ(declarations[i].name, expected[i].name) << i;
    EXPECT_EQ(declarations[i].weight, expected[i].weight) << i;
    EXPECT_EQ(declarations[i].parameter, expected[i].parameter) << i;
  }

  const auto &wire_table = EffectRegistry::WireTable();
  ASSERT_EQ(wire_table.size(), 36u);
  EXPECT_LT(wire_table.size(), 256u);
  EXPECT_EQ(EffectRegistry::RandomEffectCount(), 27u);

  size_t wire_index = 0;
  for (const auto &declaration : declarations) {
    const size_t copies = declaration.weight == 0 ? 1 : declaration.weight;
    for (size_t copy = 0; copy < copies; ++copy) {
      ASSERT_LT(wire_index, wire_table.size());
      EXPECT_EQ(wire_table[wire_index], &declaration);
      EXPECT_EQ(wire_table[wire_index]->weight, declaration.weight);
      ++wire_index;
    }
  }
  EXPECT_EQ(wire_index, wire_table.size());
  EXPECT_STREQ(wire_table[wire_table.size() - 2]->name,
               "Display Color Palette");
  EXPECT_STREQ(wire_table.back()->name, "Dark");
}

TEST(SimulatorCatalogTest, EffectFactoriesHonorExplicitSimulatorOffsets) {
  EffectSeedOverrides seeds;
  seeds.fire_offset = 6198;
  seeds.firefly_offset = 423;
  seeds.rorschach_offset = 24359;

  const auto &declarations = EffectRegistry::Declarations();
  std::unique_ptr<Effect> fire(
      EffectRegistry::CreateEffect(declarations[2], &seeds));
  std::unique_ptr<Effect> firefly(
      EffectRegistry::CreateEffect(declarations[3], &seeds));
  std::unique_ptr<Effect> rorschach(
      EffectRegistry::CreateEffect(declarations[8], &seeds));

  ASSERT_NE(fire, nullptr);
  ASSERT_NE(firefly, nullptr);
  ASSERT_NE(rorschach, nullptr);
  ExpectSameRendering(*fire, FireEffect(seeds.fire_offset));
  ExpectSameRendering(*firefly, FireflyEffect(seeds.firefly_offset));
  ExpectSameRendering(*rorschach, RorschachEffect(seeds.rorschach_offset));
}

TEST(SimulatorCatalogTest, PaletteNamesAndColorsShareOneOrderedCatalog) {
  const std::vector<const char *> expected_names = {
      "Red",          "Orange",        "Yellow",
      "Green",        "Aqua",          "Blue",
      "Purple",       "Pink",          "Rainbow",
      "Warm",         "Cool",          "Yellow-Green",
      "80s Miami",    "Vaporwave",     "Cool Popo",
      "Candy Cane",   "Winter Mint",   "Fire",
      "Pastel Rainbow", "Jazz Cup",    "Yellow & Double Purp",
      "Double Rainbow",
  };
  const std::vector<std::vector<uint32_t>> expected_colors = {
      {{0x00ffff00}},
      {{0x00ffff00 | 32u}},
      {{0x00ffff00 | 64u}},
      {{0x00ffff00 | 96u}},
      {{0x00ffff00 | 128u}},
      {{0x00ffff00 | 160u}},
      {{0x00ffff00 | 192u}},
      {{0x00ffff00 | 224u}},
      {{0x00ffff00, 0x00ffff00 | 96u, 0x00ffff00 | 160u}},
      {{0x00ffff00, 0x00ffff00 | 192u}},
      {{0x00ffff00 | 96u, 0x00ffff00 | 160u}},
      {{0x00ffff00 | 64u, 0x00ffff00 | 128u}},
      {{0x00ffff00 | 192u, 0x00ffff00 | 32u}},
      {{249u << 16 | 241u << 8 | 33u,
        255u << 16 | 188u << 8 | 247u,
        160u << 16 | 225u << 8 | 201u,
        150u << 16 | 251u << 8 | 153u}},
      {{255u << 16 | 128u, 0x00ffff00 | 160u}},
      {{0x00ffff00, 255u << 16}},
      {{0x00ffff00 | 128u, 255u << 16 | 128u}},
      {{0x00ffff00, 0x00ffff00 | 32u, 0x00ffff00 | 64u}},
      {{192u << 16 | 127u << 8,
        192u << 16 | 127u << 8 | 96u,
        192u << 16 | 127u << 8 | 160u}},
      {{0x00ffff00 | 132u, 0x00ffff00 | 192u, 200u << 16}},
      {{0x00ffff00 | 192u, 0x00ffff00 | 64u,
        0x00ffff00 | 192u}},
      {{0x00ffff00, 0x00ffff00 | 96u, 0x00ffff00 | 160u, 0x00ffff00,
        0x00ffff00 | 96u, 0x00ffff00 | 160u}},
  };

  const auto &names = Effect::palette_names();
  const auto &palettes = Effect::palettes();
  ASSERT_EQ(names.size(), expected_names.size());
  ASSERT_EQ(palettes.size(), expected_colors.size());
  std::set<std::string> unique_names;
  for (size_t palette_index = 0; palette_index < palettes.size();
       ++palette_index) {
    EXPECT_STREQ(names[palette_index], expected_names[palette_index]);
    EXPECT_TRUE(unique_names.insert(names[palette_index]).second);
    ASSERT_EQ(palettes[palette_index].Size(),
              expected_colors[palette_index].size());
    for (size_t color_index = 0;
         color_index < expected_colors[palette_index].size(); ++color_index) {
      EXPECT_EQ(PackHsv(palettes[palette_index].GetColor(color_index)),
                expected_colors[palette_index][color_index])
          << "palette " << palette_index << " color " << color_index;
    }
  }
}

struct ExpectedStrip {
  uint8_t led_count;
  uint8_t flags;
};

struct ExpectedDevice {
  const char *name;
  std::vector<ExpectedStrip> strips;
};

TEST(SimulatorCatalogTest, NamedDevicesPreserveEveryStripAndFlag) {
  const std::vector<ExpectedDevice> expected = {
      {"bike", {{30, Bright}}},
      {"ben_s_bike", {{28, Bright}}},
      {"will_bike", {{63, Bright}}},
      {"scarf", {{46, 0}}},
      {"lantern", {{5, Tiny}}},
      {"puck", {{12, Tiny | Circular}}},
      {"two_side_puck", {{24, Tiny | Circular | Mirrored}}},
      {"rainbow_cloak",
       {{11, Tiny | Circular}, {94, 0},
        {11, Tiny | Circular | Reversed}}},
      {"backpack_tail", {{11, 0}}},
      {"dan_jacket", {{60, 0}}},
      {"will_jacket", {{56, 0}}},
      {"will_bike_front", {{27, Circular}}},
      {"will_top_hat", {{50, Circular}}},
      {"bike_front", {{18, Circular}}},
      {"hex_light", {{12, Circular | Tiny}}},
      {"half_matrix_panel",
       {{16, 0}, {16, Reversed}, {16, 0}, {16, Reversed},
        {16, 0}, {16, Reversed}, {16, 0}, {16, Reversed}}},
      {"backpack_rope", {{96, Dim}, {96, Dim | Reversed}}},
      {"ufo",
       {{12, Circular}, {16, Circular}, {12, Bright | Circular},
        {52, Dim | Circular}}},
      {"brooke_bike",
       {{15, Circular}, {19, 0}, {16, Circular}, {33, Bright}}},
      {"ross_backpack",
       {{13, Reversed}, {12, 0}, {13, Reversed}, {12, 0}}},
      {"whatever", {{18, Circular}}},
      {"will_backpack", {{96, 0}, {96, Reversed}}},
  };

  const auto &devices = DeviceCatalog::All();
  ASSERT_EQ(devices.size(), expected.size());
  std::set<std::string> names;
  for (size_t device_index = 0; device_index < devices.size(); ++device_index) {
    const auto &actual = devices[device_index];
    EXPECT_STREQ(actual.name, expected[device_index].name);
    EXPECT_TRUE(names.insert(actual.name).second);
    ASSERT_NE(actual.description, nullptr);
    EXPECT_EQ(actual.description->milliamps_supported, 2350u);
    ASSERT_EQ(actual.description->strips.size(),
              expected[device_index].strips.size());
    for (size_t strip_index = 0;
         strip_index < actual.description->strips.size(); ++strip_index) {
      const StripDescription &strip = actual.description->strips[strip_index];
      EXPECT_EQ(strip.led_count,
                expected[device_index].strips[strip_index].led_count);
      EXPECT_EQ(strip.GetFlags(),
                expected[device_index].strips[strip_index].flags);
    }
  }

  ASSERT_NE(DeviceCatalog::Find("ufo"), nullptr);
  EXPECT_EQ(DeviceCatalog::Find("ufo"), &devices[17]);
  EXPECT_EQ(DeviceCatalog::Find("not-a-device"), nullptr);
}

}  // namespace
