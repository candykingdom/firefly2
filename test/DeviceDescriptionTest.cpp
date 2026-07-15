#include <DeviceDescription.hpp>

#include "gtest/gtest.h"

TEST(DeviceDescription, ledCountSumsZeroOneAndManyStrips) {
  EXPECT_EQ(DeviceDescription(2000, {}).GetLedCount(), 0);
  EXPECT_EQ(DeviceDescription(2000, {StripDescription(7, {})}).GetLedCount(),
            7);
  EXPECT_EQ(DeviceDescription(2000, {StripDescription(3, {}),
                                     StripDescription(5, {}),
                                     StripDescription(2, {})})
                .GetLedCount(),
            10);
}

// Pins documented current behavior, not a fix: GetLedCount accumulates into a
// uint16_t but returns uint8_t, so devices past 255 total LEDs truncate
// mod 256 (known latent limitation - the whole render path indexes LEDs with
// uint8_t). Changing this is a separate decision; this test exists so the
// change is deliberate.
TEST(DeviceDescription, ledCountAbove255TruncatesToUint8) {
  DeviceDescription device(2000,
                           {StripDescription(200, {}), StripDescription(200, {})});
  EXPECT_EQ(device.GetLedCount(), (200 + 200) % 256);
}
