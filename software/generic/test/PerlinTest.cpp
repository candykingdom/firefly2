#include <Perlin.hpp>

#include "gtest/gtest.h"

static const uint16_t tolerance = 8;

TEST(Perlin, shouldProduceContiguousNoise) {
  for (uint8_t i = 0; i < 255; ++i) {
    uint32_t x = random();
    uint32_t y = random();

    uint16_t first = perlinNoise(x, y);
    uint16_t second = perlinNoise(x + 1, y + 1);
    ASSERT_LE(abs(first - second), tolerance) << "Tested " << x << ", " << y;
  }
}

TEST(Perlin, shouldWrapAround) {
  for (uint8_t i = 0; i < 128; ++i) {
    uint32_t y = random();
    uint16_t start = perlinNoise(0, y);
    uint16_t end = perlinNoise(4294967295, y);
    ASSERT_LE(abs(start - end), tolerance)
        << "Tested " << 4294967295 << ", " << y;
  }

  for (uint8_t i = 0; i < 128; ++i) {
    uint32_t x = random();
    uint16_t start = perlinNoise(x, 0);
    uint16_t end = perlinNoise(x, 4294967295);
    ASSERT_LE(abs(start - end), tolerance)
        << "Tested " << x << ", " << 4294967295;
  }
}

TEST(Perlin, shouldProduceContiguousNoiseLinearly) {
  for (uint8_t i = 0; i < 255; ++i) {
    uint32_t x = random();
    uint32_t y = random();
    uint32_t angle = random();

    uint16_t first = perlinNoisePolar(x, y, angle, i);
    uint16_t second = perlinNoisePolar(x, y, angle, i + 1);
    ASSERT_LE(abs(first - second), tolerance);
  }
}

TEST(Perlin, shouldProduceContiguousNoiseRadially) {
  for (uint8_t i = 0; i < 255; ++i) {
    uint32_t x = random();
    uint32_t y = random();
    uint8_t magnitude = random();
    // The distance between points radially can get very large, make sure the
    // magnitude isn't too high!
    magnitude /= 2;

    uint16_t first = perlinNoisePolar(x, y, i, magnitude);
    uint16_t second = perlinNoisePolar(x, y, i + 1, magnitude);
    ASSERT_LE(abs(first - second), tolerance);
  }
}

TEST(Perlin, shouldWrapAroundPolar) {
  for (uint8_t i = 0; i < 255; ++i) {
    uint32_t x = random();
    uint32_t y = random();
    uint8_t magnitude = random();
    // The distance between points radially can get very large, make sure the
    // magnitude isn't too high!
    magnitude /= 2;

    uint16_t first = perlinNoisePolar(x, y, 0, magnitude);
    uint16_t second = perlinNoisePolar(x, y, 255, magnitude);
    ASSERT_LE(abs(first - second), tolerance);
  }
}