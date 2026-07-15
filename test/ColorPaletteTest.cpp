#include <ColorPalette.hpp>
#include <Effect.hpp>

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

// Degenerate inputs (G4): empty and single-color palettes must not
// interpolate.
TEST(ColorPalette, gradientOfEmptyPaletteIsBlack) {
  ColorPalette p{};
  EXPECT_EQ(p.Size(), 0);

  for (fract16 position : {(fract16)0, (fract16)(MAX_UINT16 / 2),
                           (fract16)MAX_UINT16}) {
    CHSV color = p.GetGradient(position);
    EXPECT_EQ(color.h, 0) << "position " << position;
    EXPECT_EQ(color.s, 0) << "position " << position;
    EXPECT_EQ(color.v, 0) << "position " << position;
  }
}

TEST(ColorPalette, gradientOfSingleColorPaletteIsThatColorEverywhere) {
  ColorPalette p{{HUE_AQUA, 200, 150}};

  for (fract16 position : {(fract16)0, (fract16)1, (fract16)(MAX_UINT16 / 2),
                           (fract16)MAX_UINT16}) {
    for (bool wrap : {true, false}) {
      CHSV color = p.GetGradient(position, wrap);
      EXPECT_EQ(color.h, HUE_AQUA) << "position " << position;
      EXPECT_EQ(color.s, 200) << "position " << position;
      EXPECT_EQ(color.v, 150) << "position " << position;
    }
  }
}

// Positions that land exactly on a palette entry return that entry with no
// interpolation, and with wrap the max position comes back around to the
// first color.
TEST(ColorPalette, gradientAtExactColorBoundariesReturnsThatColor) {
  ColorPalette p{
      {HUE_RED, 255, 255},
      {HUE_GREEN, 200, 150},
      {HUE_BLUE, 100, 50},
  };

  const uint16_t color_range = MAX_UINT16 / 3;  // wrap=true segment width
  for (uint8_t i = 0; i < 3; i++) {
    CHSV expected = p.GetColor(i);
    CHSV actual = p.GetGradient(color_range * i);
    EXPECT_EQ(actual.h, expected.h) << "boundary " << (int)i;
    EXPECT_EQ(actual.s, expected.s) << "boundary " << (int)i;
    EXPECT_EQ(actual.v, expected.v) << "boundary " << (int)i;
  }

  // color_range * 3 == 65535: with wrap, the end of the last segment is the
  // first color again.
  CHSV wrapped = p.GetGradient(color_range * 3);
  EXPECT_EQ(wrapped.h, HUE_RED);
}

// Registry boundary (G4): palette indices are single wire bytes validated
// only against this table's size. The corpus records 22 palettes; an
// out-of-range index reaching palettes()[index] is out-of-bounds vector
// access (undefined behavior - a known latent defect flagged in the 002
// audit review, deliberately NOT exercised or fixed here). This pins the
// boundary so a table-size change is a conscious, corpus-regenerating one.
TEST(ColorPalette, effectPaletteRegistryBoundary) {
  ASSERT_EQ(Effect::palettes().size(), 22u);
  for (const ColorPalette &palette : Effect::palettes()) {
    EXPECT_GE(palette.Size(), 1) << "palettes must never be empty";
  }
}
