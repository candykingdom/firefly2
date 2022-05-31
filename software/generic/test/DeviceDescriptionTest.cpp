#include "../StripDescription.hpp"
#include "gtest/gtest.h"

TEST(StripDescription, usesFlagsCorrectly) {
  StripDescription sd = StripDescription(10, {Bright, Tiny});

  EXPECT_EQ(sd.led_count, 10);

  EXPECT_TRUE(sd.FlagEnabled(Bright));
  EXPECT_TRUE(sd.FlagEnabled(Tiny));
  EXPECT_FALSE(sd.FlagEnabled(Circular));
}
