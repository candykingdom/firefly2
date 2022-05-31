#include <ColorPalette.hpp>

#include "gtest/gtest.h"

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

  CHSV one_fourth = p.GetGradient(MAX_UINT16 / 4);
  EXPECT_EQ(one_fourth.h, HUE_GREEN / 2 - 1);
  EXPECT_EQ(one_fourth.s, 255 / 2);
  EXPECT_EQ(one_fourth.v, 255 / 2);

  CHSV one_half = p.GetGradient(MAX_UINT16 / 2);
  EXPECT_EQ(one_half.h, HUE_GREEN);
  EXPECT_EQ(one_half.s, 255);
  EXPECT_EQ(one_half.v, 255);

  CHSV three_fourths = p.GetGradient(MAX_UINT16 * 3 / 4);
  EXPECT_EQ(three_fourths.h, HUE_GREEN / 2);
  EXPECT_EQ(three_fourths.s, 256 / 2);
  EXPECT_EQ(three_fourths.v, 256 / 2);
}

TEST(ColorPalette, lerpOddNumberOfValues) {
  ColorPalette p{
      {HUE_RED, 255, 255},
      {HUE_GREEN, 255, 255},
      {HUE_BLUE, 255, 255},
  };

  CHSV one_sixth = p.GetGradient(MAX_UINT16 / 6);
  EXPECT_EQ(one_sixth.h, 47);  // Yellow

  CHSV one_half = p.GetGradient(MAX_UINT16 / 2);
  EXPECT_EQ(one_half.h, 127);  // Cyan

  CHSV five_sixths = p.GetGradient(MAX_UINT16 * 5 / 6);
  EXPECT_EQ(five_sixths.h, 207);  // Magenta
}

TEST(ColorPalette, wrapHueRedBlue) {
  uint8_t violet = (256 - (256 - HUE_BLUE) / 2);

  ColorPalette p1{
      {HUE_RED, 255, 255},
      {HUE_BLUE, 255, 255},
  };

  CHSV one_fourth_p1 = p1.GetGradient(MAX_UINT16 / 4);
  EXPECT_EQ(one_fourth_p1.h, violet) << "Red to blue.";

  ColorPalette p2{
      {HUE_BLUE, 255, 255},
      {HUE_RED, 255, 255},
  };

  CHSV one_fourth_p2 = p2.GetGradient(MAX_UINT16 / 4);
  EXPECT_EQ(one_fourth_p2.h, violet - 1) << "Blue to red.";
}

TEST(ColorPalette, wrapHueOrangePurple) {
  uint8_t mid = 234;

  ColorPalette p{
      {HUE_ORANGE, 255, 255},
      {HUE_PURPLE, 255, 255},
  };

  CHSV one_fourth = p.GetGradient(MAX_UINT16 / 4, false);
  EXPECT_EQ(one_fourth.h, 9) << "Orange to purple.";

  CHSV one_half = p.GetGradient(MAX_UINT16 / 2, false);
  EXPECT_EQ(one_half.h, 240) << "Orange to purple.";

  CHSV three_fourths = p.GetGradient(MAX_UINT16 / 4 * 3, false);
  EXPECT_EQ(three_fourths.h, 216) << "Orange to purple.";
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
