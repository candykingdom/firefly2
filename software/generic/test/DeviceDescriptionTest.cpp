#include <LedManager.hpp>

#include "../DeviceDescription.hpp"
#include "gtest/gtest.h"

TEST(DeviceDescription, usesFlagsCorrectly) {
  DeviceDescription dd = DeviceDescription(10, {Bright, Tiny});

  EXPECT_EQ(dd.led_count, 10);

  EXPECT_TRUE(dd.FlagEnabled(Bright));
  EXPECT_TRUE(dd.FlagEnabled(Tiny));
  EXPECT_FALSE(dd.FlagEnabled(Circular));
}
