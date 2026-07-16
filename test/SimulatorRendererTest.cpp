#include "../sim/wasm/SimulatorBridge.hpp"

#include <StripDescription.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

std::vector<uint8_t> CopyOutput(uint32_t handle) {
  const uint32_t size = ff_sim_render_size(handle);
  const uint8_t *data = ff_sim_render_data(handle);
  if (size == 0) {
    EXPECT_EQ(data, nullptr);
    return {};
  }
  EXPECT_NE(data, nullptr);
  return std::vector<uint8_t>(data, data + size);
}

uint32_t FindDevice(const char *name) {
  for (uint32_t i = 0; i < ff_sim_device_count(); ++i) {
    if (std::strcmp(ff_sim_device_name(i), name) == 0) {
      return i;
    }
  }
  return ff_sim_device_count();
}

TEST(SimulatorRendererTest, MetadataMatchesAuthoritativeCatalogs) {
  EXPECT_EQ(ff_sim_abi_version(), 1u);

  ASSERT_EQ(ff_sim_effect_count(), 35u);
  EXPECT_EQ(ff_sim_random_effect_count(), 27u);
  EXPECT_STREQ(ff_sim_effect_name(0), "Color Cycle");
  EXPECT_EQ(ff_sim_effect_weight(0), 2u);
  EXPECT_STREQ(ff_sim_effect_name(33), "Display Color Palette");
  EXPECT_EQ(ff_sim_effect_weight(33), 0u);
  EXPECT_STREQ(ff_sim_effect_name(34), "Dark");
  EXPECT_EQ(ff_sim_effect_name(35), nullptr);
  EXPECT_EQ(ff_sim_effect_weight(35), 0u);

  ASSERT_EQ(ff_sim_palette_count(), 22u);
  EXPECT_STREQ(ff_sim_palette_name(0), "Red");
  EXPECT_EQ(ff_sim_palette_color_count(0), 1u);
  EXPECT_EQ(ff_sim_palette_color_hsv(0, 0), 0x00ffff00u);
  EXPECT_STREQ(ff_sim_palette_name(13), "Vaporwave");
  EXPECT_EQ(ff_sim_palette_color_count(13), 4u);
  EXPECT_EQ(ff_sim_palette_color_hsv(13, 0),
            249u << 16 | 241u << 8 | 33u);
  EXPECT_EQ(ff_sim_palette_name(22), nullptr);
  EXPECT_EQ(ff_sim_palette_color_count(22), 0u);
  EXPECT_EQ(ff_sim_palette_color_hsv(22, 0), 0u);
  EXPECT_EQ(ff_sim_palette_color_hsv(0, 1), 0u);

  ASSERT_EQ(ff_sim_device_count(), 22u);
  const uint32_t ufo = FindDevice("ufo");
  ASSERT_LT(ufo, ff_sim_device_count());
  EXPECT_EQ(ff_sim_device_milliamps(ufo), 2350u);
  ASSERT_EQ(ff_sim_device_strip_count(ufo), 4u);
  EXPECT_EQ(ff_sim_strip_led_count(ufo, 0), 12u);
  EXPECT_EQ(ff_sim_strip_flags(ufo, 0), Circular);
  EXPECT_EQ(ff_sim_strip_led_count(ufo, 3), 52u);
  EXPECT_EQ(ff_sim_strip_flags(ufo, 3), Dim | Circular);
  EXPECT_EQ(ff_sim_device_name(22), nullptr);
  EXPECT_EQ(ff_sim_device_milliamps(22), 0u);
  EXPECT_EQ(ff_sim_device_strip_count(22), 0u);
  EXPECT_EQ(ff_sim_strip_led_count(ufo, 4), 0u);
  EXPECT_EQ(ff_sim_strip_flags(ufo, 4), 0u);
}

TEST(SimulatorRendererTest, HandlesOwnOutputAndRejectInvalidLifecycleCalls) {
  EXPECT_EQ(ff_sim_renderer_destroy(0), -1);
  EXPECT_EQ(ff_sim_renderer_destroy(123456), -1);
  EXPECT_EQ(ff_sim_render_device(0, 0, 0, 0, 0, 0, 0), -1);
  EXPECT_EQ(ff_sim_render_strip(0, 1, 0, 0, 0, 0, 0, 0), -1);
  EXPECT_EQ(ff_sim_render_size(0), 0u);
  EXPECT_EQ(ff_sim_render_data(0), nullptr);

  const uint32_t first = ff_sim_renderer_create(6198, 423, 24359);
  const uint32_t second = ff_sim_renderer_create(1, 2, 3);
  ASSERT_NE(first, 0u);
  ASSERT_NE(second, 0u);
  EXPECT_NE(first, second);
  EXPECT_EQ(ff_sim_render_size(first), 0u);
  EXPECT_EQ(ff_sim_render_data(first), nullptr);

  EXPECT_EQ(ff_sim_render_device(first, ff_sim_device_count(), 0, 0, 0, 0,
                                 0),
            -2);
  EXPECT_EQ(ff_sim_renderer_destroy(first), 0);
  EXPECT_EQ(ff_sim_renderer_destroy(first), -1);
  EXPECT_EQ(ff_sim_render_device(first, 0, 0, 0, 0, 0, 0), -1);
  EXPECT_EQ(ff_sim_renderer_destroy(second), 0);
}

TEST(SimulatorRendererTest, DeviceRenderReturnsPackedRgbBytes) {
  const uint32_t handle = ff_sim_renderer_create(6198, 423, 24359);
  ASSERT_NE(handle, 0u);
  const uint32_t scarf = FindDevice("scarf");
  ASSERT_LT(scarf, ff_sim_device_count());

  ASSERT_EQ(ff_sim_render_device(handle, scarf, 13, 8, 1000, 0, 0), 0);
  const std::vector<uint8_t> rainbow = CopyOutput(handle);
  ASSERT_EQ(rainbow.size(), 46u * 3u);
  EXPECT_TRUE(std::any_of(rainbow.begin(), rainbow.end(),
                          [](uint8_t channel) { return channel != 0; }));

  const uint32_t packed_rgb = 12u | 34u << 8 | 56u << 16;
  ASSERT_EQ(ff_sim_render_device(handle, scarf, 255, 255, 0xffffffffu, 1,
                                 packed_rgb),
            0);
  const std::vector<uint8_t> controlled = CopyOutput(handle);
  ASSERT_EQ(controlled.size(), 46u * 3u);
  for (size_t i = 0; i < controlled.size(); i += 3) {
    EXPECT_EQ(controlled[i], 12u);
    EXPECT_EQ(controlled[i + 1], 34u);
    EXPECT_EQ(controlled[i + 2], 56u);
  }

  EXPECT_EQ(ff_sim_renderer_destroy(handle), 0);
}

TEST(SimulatorRendererTest, CustomStripUsesProductionFlagsAndByteWrapping) {
  const uint32_t handle = ff_sim_renderer_create(6198, 423, 24359);
  ASSERT_NE(handle, 0u);

  ASSERT_EQ(ff_sim_render_strip(handle, 12, Circular, 0, 0, 1234, 0, 0), 0);
  const std::vector<uint8_t> base = CopyOutput(handle);
  ASSERT_EQ(base.size(), 36u);
  ASSERT_EQ(
      ff_sim_render_strip(handle, 12, Circular, 256, 256, 1234, 0, 0), 0);
  EXPECT_EQ(CopyOutput(handle), base);

  ASSERT_EQ(
      ff_sim_render_strip(handle, 12, Circular, 255, 255, 1234, 0, 0), 0);
  const std::vector<uint8_t> invalid_bytes = CopyOutput(handle);
  ASSERT_EQ(ff_sim_render_strip(handle, 12, Circular, 255 % 35, 255 % 22,
                                1234, 0, 0),
            0);
  EXPECT_EQ(CopyOutput(handle), invalid_bytes);

  const uint32_t packed_rgb = 80u | 40u << 8 | 16u << 16;
  ASSERT_EQ(ff_sim_render_strip(handle, 3, Dim | Reversed, 0, 0, 0, 1,
                                packed_rgb),
            0);
  EXPECT_EQ(CopyOutput(handle),
            (std::vector<uint8_t>{10, 5, 2, 10, 5, 2, 10, 5, 2}));

  ASSERT_EQ(ff_sim_render_strip(handle, 3, Off, 0, 0, 0, 1, packed_rgb), 0);
  EXPECT_EQ(CopyOutput(handle), (std::vector<uint8_t>(9, 0)));

  ASSERT_EQ(ff_sim_render_strip(handle, 0, 0, 0, 0, 0, 0, 0), 0);
  EXPECT_TRUE(CopyOutput(handle).empty());
  ASSERT_EQ(ff_sim_render_strip(handle, 255, 0, 34, 21, 0xffffffffu, 0, 0),
            0);
  EXPECT_EQ(CopyOutput(handle).size(), 255u * 3u);

  EXPECT_EQ(ff_sim_renderer_destroy(handle), 0);
}

}  // namespace
