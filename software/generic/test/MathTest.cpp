#include "../Math.hpp"
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

TEST(Math, shouldMirrorIndiciesOdd) {
  uint8_t led_index;
  uint8_t led_count;

  led_index = 0;
  led_count = 5;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 0);
  ASSERT_EQ(led_count, 3);

  led_index = 2;
  led_count = 5;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 2);
  ASSERT_EQ(led_count, 3);

  led_index = 4;
  led_count = 5;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 0);
  ASSERT_EQ(led_count, 3);
}

TEST(Math, shouldMirrorIndiciesEven) {
  uint8_t led_index;
  uint8_t led_count;

  led_index = 0;
  led_count = 4;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 0);
  ASSERT_EQ(led_count, 2);

  led_index = 1;
  led_count = 4;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 1);
  ASSERT_EQ(led_count, 2);

  led_index = 2;
  led_count = 4;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 1);
  ASSERT_EQ(led_count, 2);

  led_index = 3;
  led_count = 4;
  MirrorIndex(&led_index, &led_count);
  ASSERT_EQ(led_index, 0);
  ASSERT_EQ(led_count, 2);
}
