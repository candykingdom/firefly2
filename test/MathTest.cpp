#include <Math.hpp>
#include <string>

#include "gtest/gtest.h"

TEST(Math, shouldGetCardinalCordinates) {
  uint8_t angle;
  uint8_t radius;

  // By 4s
  GetPosOnCircle(4, 0, &angle, &radius);
  ASSERT_EQ(angle, 0);
  ASSERT_EQ(radius, 20);

  GetPosOnCircle(4, 1, &angle, &radius);
  ASSERT_EQ(angle, 63);
  ASSERT_EQ(radius, 20);

  GetPosOnCircle(4, 2, &angle, &radius);
  ASSERT_EQ(angle, 127);
  ASSERT_EQ(radius, 20);

  GetPosOnCircle(4, 3, &angle, &radius);
  ASSERT_EQ(angle, 191);
  ASSERT_EQ(radius, 20);

  // By 8s
  GetPosOnCircle(8, 0, &angle, &radius);
  ASSERT_EQ(angle, 0);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 1, &angle, &radius);
  ASSERT_EQ(angle, 31);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 2, &angle, &radius);
  ASSERT_EQ(angle, 63);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 3, &angle, &radius);
  ASSERT_EQ(angle, 95);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 4, &angle, &radius);
  ASSERT_EQ(angle, 127);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 5, &angle, &radius);
  ASSERT_EQ(angle, 159);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 6, &angle, &radius);
  ASSERT_EQ(angle, 191);
  ASSERT_EQ(radius, 40);

  GetPosOnCircle(8, 7, &angle, &radius);
  ASSERT_EQ(angle, 223);
  ASSERT_EQ(radius, 40);
}

TEST(Math, shouldMirrorIndicies) {
  std::vector<std::vector<uint8_t>> cases = {
      {0, 5, 0, 3}, {1, 5, 1, 3}, {2, 5, 2, 3}, {3, 5, 1, 3}, {4, 5, 0, 3},
      {0, 4, 0, 2}, {1, 4, 1, 2}, {2, 4, 1, 2}, {3, 4, 0, 2},
  };

  for (auto it = cases.begin(); it != cases.end(); ++it) {
    uint16_t led_index = (*it)[0];
    uint16_t led_count = (*it)[1];
    MirrorIndex(&led_index, &led_count);
    std::string run_desc = "(index: " + std::to_string((*it)[0]) +
                           ", count: " + std::to_string((*it)[1]) + ")";
    ASSERT_EQ(led_index, (*it)[2])
        << "Expected mirrored index is incorrect! " + run_desc;
    ASSERT_EQ(led_count, (*it)[3])
        << "Expected mirrored LED count is incorrect! " + run_desc;
  }
}
