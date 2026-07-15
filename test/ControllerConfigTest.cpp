#include "../src/devices/controller/config-utils.h"
#include "gtest/gtest.h"

TEST(ControllerConfigTest, MapsButtonRowsToAlternatingSlots) {
  EXPECT_EQ(ButtonRowToSlot(0, false), 0);
  EXPECT_EQ(ButtonRowToSlot(0, true), 1);
  EXPECT_EQ(ButtonRowToSlot(1, false), 2);
  EXPECT_EQ(ButtonRowToSlot(1, true), 3);
  EXPECT_EQ(ButtonRowToSlot(2, false), 4);
  EXPECT_EQ(ButtonRowToSlot(2, true), 5);
}

TEST(ControllerConfigTest, PreviousPaletteIndexWrapsAtZero) {
  EXPECT_EQ(PreviousPaletteIndex(1, 22), 0);
  EXPECT_EQ(PreviousPaletteIndex(0, 22), 21);
  EXPECT_EQ(PreviousPaletteIndex(0, 1), 0);
  EXPECT_EQ(PreviousPaletteIndex(7, 0), 7);
}
