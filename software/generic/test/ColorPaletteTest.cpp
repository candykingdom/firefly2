#include "gtest/gtest.h"

#include "../ColorPalette.hpp"

TEST(ColorPalette, indexColors) {
  ColorPalette p{
      {HUE_RED, 255, 255},
      {HUE_GREEN, 255, 255},
      {HUE_BLUE, 255, 255},
  };

  EXPECT_EQ(p.Size(), 3);
  EXPECT_EQ(p.GetColor(0).h, HUE_RED);
  EXPECT_EQ(p.GetColor(1).h, HUE_GREEN);
  EXPECT_EQ(p.GetColor(2).h, HUE_BLUE);
  EXPECT_EQ(p.GetColor(3).h, HUE_RED);
}

TEST(ColorPalette, lerpValues) {
  ColorPalette p{
      {HUE_RED, 0, 0},
      {HUE_GREEN, 255, 255},
  };

  CHSV oneFourth = p.GetGradient(MAX_UINT16 / 4);
  EXPECT_EQ(oneFourth.h, HUE_GREEN / 2 - 1);
  EXPECT_EQ(oneFourth.s, 255 / 2);
  EXPECT_EQ(oneFourth.v, 255 / 2);

  CHSV oneHalf = p.GetGradient(MAX_UINT16 / 2);
  EXPECT_EQ(oneHalf.h, HUE_GREEN);
  EXPECT_EQ(oneHalf.s, 255);
  EXPECT_EQ(oneHalf.v, 255);

  CHSV threeFourths = p.GetGradient(MAX_UINT16 * 3 / 4);
  EXPECT_EQ(threeFourths.h, HUE_GREEN / 2);
  EXPECT_EQ(threeFourths.s, 256 / 2);
  EXPECT_EQ(threeFourths.v, 256 / 2);
}

TEST(ColorPalette, lerpOddNumberOfValues) {
  ColorPalette p{
      {HUE_RED, 255, 255},
      {HUE_GREEN, 255, 255},
      {HUE_BLUE, 255, 255},
  };

  CHSV oneSixth = p.GetGradient(MAX_UINT16 / 6);
  EXPECT_EQ(oneSixth.h, 47);  // Yellow

  CHSV oneHalf = p.GetGradient(MAX_UINT16 / 2);
  EXPECT_EQ(oneHalf.h, 127);  // Cyan

  CHSV fiveSixths = p.GetGradient(MAX_UINT16 * 5 / 6);
  EXPECT_EQ(fiveSixths.h, 207);  // Magenta
}

TEST(ColorPalette, wrapHueRedBlue) {
  uint8_t violet = (256 - (256 - HUE_BLUE) / 2);

  ColorPalette p1{
      {HUE_RED, 255, 255},
      {HUE_BLUE, 255, 255},
  };

  CHSV oneFourthP1 = p1.GetGradient(MAX_UINT16 / 4);
  EXPECT_EQ(oneFourthP1.h, violet) << "Red to blue.";

  ColorPalette p2{
      {HUE_BLUE, 255, 255},
      {HUE_RED, 255, 255},
  };

  CHSV oneFourthP2 = p2.GetGradient(MAX_UINT16 / 4);
  EXPECT_EQ(oneFourthP2.h, violet - 1) << "Blue to red.";
}

TEST(ColorPalette, wrapHueOrangePurple) {
  uint8_t mid = 234;

  ColorPalette p{
      {HUE_ORANGE, 255, 255},
      {HUE_PURPLE, 255, 255},
  };

  CHSV oneFourth = p.GetGradient(MAX_UINT16 / 4, false);
  EXPECT_EQ(oneFourth.h, 9) << "Orange to purple.";

  CHSV oneHalf = p.GetGradient(MAX_UINT16 / 2, false);
  EXPECT_EQ(oneHalf.h, 240) << "Orange to purple.";

  CHSV threeFourths = p.GetGradient(MAX_UINT16 / 4 * 3, false);
  EXPECT_EQ(threeFourths.h, 216) << "Orange to purple.";
}

TEST(ColorPalette, wrapHueNotValues) {
  ColorPalette p{
      {HUE_RED, 255, 255},
      {HUE_BLUE, 255, 255},
  };

  CHSV first = p.GetGradient(0, false);
  EXPECT_EQ(first.h, HUE_RED);

  uint8_t violet = (256 - (256 - HUE_BLUE) / 2);
  CHSV half = p.GetGradient(MAX_UINT16 / 2, false);
  EXPECT_EQ(half.h, violet);

  CHSV last = p.GetGradient(MAX_UINT16, false);
  EXPECT_EQ(last.h, HUE_BLUE);
}
