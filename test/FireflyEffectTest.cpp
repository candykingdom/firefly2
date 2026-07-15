#include <FireflyEffect.hpp>
#include <StripDescription.hpp>

#include "gtest/gtest.h"

// Tests for defect D3 of specs/002-fix-audit-findings: FireflyEffect's
// Controller-strip offset computed `(kBlinkPeriod + 1234) << led_index`,
// which is undefined behavior for led_index >= 32. The fix masks the shift
// count, which must be a no-op for indices < 32.

// Pins exact pre-fix outputs (captured at commit 220ac6f) for every index
// the shift was defined for, proving the masked shift is bit-identical on
// all current hardware. Controller-strip output has no random state: odd
// indices are black, even indices derive their offset from led_index alone.
// time_ms 47500 is in the offset-sensitive in-sync -> out-of-sync phase, so
// a change to the offset computation would change these values.
TEST(FireflyEffect, maskedShiftKeepsOutputIdenticalBelow32) {
  RadioPacket packet;
  packet.writeSetEffect(0, 0, 8);
  StripDescription strip(64, {Controller});
  FireflyEffect effect;

  const CRGB expected[32] = {
      {37, 30, 0}, {0, 0, 0}, {0, 0, 0},   {0, 0, 0},  {1, 1, 0},   {0, 0, 0},
      {31, 24, 0}, {0, 0, 0}, {19, 15, 0}, {0, 0, 0},  {19, 15, 0}, {0, 0, 0},
      {19, 15, 0}, {0, 0, 0}, {19, 15, 0}, {0, 0, 0},  {19, 15, 0}, {0, 0, 0},
      {19, 15, 0}, {0, 0, 0}, {19, 15, 0}, {0, 0, 0},  {19, 15, 0}, {0, 0, 0},
      {19, 15, 0}, {0, 0, 0}, {19, 15, 0}, {0, 0, 0},  {19, 15, 0}, {0, 0, 0},
      {19, 15, 0}, {0, 0, 0},
  };

  for (uint8_t led = 0; led < 32; led++) {
    CRGB rgb = effect.GetRGB(led, 47500, strip, &packet);
    EXPECT_EQ(expected[led].r, rgb.r) << "led " << (int)led;
    EXPECT_EQ(expected[led].g, rgb.g) << "led " << (int)led;
    EXPECT_EQ(expected[led].b, rgb.b) << "led " << (int)led;
  }
}

// Every LED index on a max-size Controller strip must render with defined
// behavior. Pre-fix, index 32 shifts by >= the operand width and UBSan
// aborts here.
TEST(FireflyEffect, allIndicesWellDefinedOnMaxControllerStrip) {
  RadioPacket packet;
  packet.writeSetEffect(0, 0, 8);
  StripDescription strip(255, {Controller});
  FireflyEffect effect;

  for (uint32_t time : {0u, 25000u, 47500u, 123456u}) {
    for (uint16_t led = 0; led < 256; led++) {
      effect.GetRGB(led, time, strip, &packet);
    }
  }
}
