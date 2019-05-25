#include "gtest/gtest.h"

#include "../Perlin.hpp"

static const uint16_t tolerance = 8;

TEST(Perlin, shouldProduceContiguousNoise) {
  for (uint8_t i = 0; i < 255; ++i) {
    uint32_t x = random();
    uint32_t y = random();

    uint16_t first = perlinNoise(x, y);
    uint16_t second = perlinNoise(x + 1, y + 1);
    ASSERT_TRUE(abs(first - second) <= tolerance)
        << "Tested " << x << ", " << y;
  }
}

TEST(Perlin, shouldWrapAround) {
  for (uint8_t i = 0; i < 128; ++i) {
    uint32_t y = random();
    uint16_t start = perlinNoise(0, y);
    uint16_t end = perlinNoise(4294967295, y);
    ASSERT_TRUE(abs(start - end) <= tolerance)
        << "Tested " << 4294967295 << ", " << y;
  }

  for (uint8_t i = 0; i < 128; ++i) {
    uint32_t x = random();
    uint16_t start = perlinNoise(x, 0);
    uint16_t end = perlinNoise(x, 4294967295);
    ASSERT_TRUE(abs(start - end) <= tolerance)
        << "Tested " << x << ", " << 4294967295;
  }
}