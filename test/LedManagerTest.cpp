#include <DeviceDescription.hpp>
#include <LedManager.hpp>

#include "Effect.hpp"
#include "Effects.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

TEST(LedManager, hasNonRandomEffects) {
  StripDescription strip = StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {strip});
  EXPECT_EQ(device.GetLedCount(), 1);
  FakeRadio radio;
  NetworkManager networkManager = NetworkManager(&radio);
  RadioStateMachine state_machine = RadioStateMachine(&networkManager);
  FakeLedManager manager = FakeLedManager(device, &state_machine);
  manager.ClearEffects();
  manager.PublicAddEffect(new SimpleBlinkEffect(10), 4);
  manager.PublicAddEffect(new PoliceEffect(), 0);
  manager.PublicAddEffect(new FireEffect(), 2);

  EXPECT_EQ(manager.GetNumEffects(), 6);
  EXPECT_EQ(manager.GetNumUniqueEffects(), 3);

  EXPECT_EQ(manager.UniqueEffectNumberToIndex(0), 0);
  EXPECT_EQ(manager.UniqueEffectNumberToIndex(1), 4);
  EXPECT_EQ(manager.UniqueEffectNumberToIndex(2), 6);

  RadioPacket setEffect;
  setEffect.writeSetEffect(0, 0, 0);

  Effect *effect1 = manager.GetEffect(0);
  Effect *alsoEffect1 = manager.GetEffect(1);
  Effect *effect2 = manager.GetEffect(4);
  Effect *effect3 = manager.GetEffect(6);
  EXPECT_EQ(effect1->GetRGB(0, 0, strip, &setEffect),
            alsoEffect1->GetRGB(0, 0, strip, &setEffect));
  EXPECT_NE(effect1->GetRGB(0, 0, strip, &setEffect),
            effect2->GetRGB(0, 0, strip, &setEffect));
  // SimpleBlinkEffect and PoliceEffect have the same color at t=0
  EXPECT_NE(effect1->GetRGB(0, 15, strip, &setEffect),
            effect3->GetRGB(0, 15, strip, &setEffect));
  EXPECT_NE(effect2->GetRGB(0, 0, strip, &setEffect),
            effect3->GetRGB(0, 0, strip, &setEffect));
}

TEST(LedManager, effectIndexIsInRange) {
  StripDescription strip = StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {strip});
  EXPECT_EQ(device.GetLedCount(), 1);
  FakeRadio radio;
  NetworkManager networkManager = NetworkManager(&radio);
  RadioStateMachine state_machine = RadioStateMachine(&networkManager);
  // The calls to AddEffect in LedManager's constructor validate that the number
  // of effects is in range.
  FakeLedManager led_manager = FakeLedManager(device, &state_machine);
}

class TestEffect : public Effect {
 public:
  TestEffect() {}

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip, RadioPacket *setEffectPacket) const {
    UNUSED(time_ms);
    UNUSED(strip);
    UNUSED(setEffectPacket);
    return CRGB(led_index, 0, 0);
  }
};

TEST(LedManager, callStripInReverse) {
  StripDescription strip = StripDescription(5, {Reversed});
  DeviceDescription device = DeviceDescription(2000, {strip});
  EXPECT_EQ(device.GetLedCount(), 5);
  FakeRadio radio;
  NetworkManager networkManager = NetworkManager(&radio);
  RadioStateMachine state_machine = RadioStateMachine(&networkManager);
  FakeLedManager manager = FakeLedManager(device, &state_machine);

  manager.ClearEffects();
  TestEffect *test_effect = new TestEffect();
  manager.PublicAddEffect(test_effect, 1);

  RadioPacket setEffect;
  setEffect.writeSetEffect(0, 0, 0);
  state_machine.SetEffect(&setEffect);

  manager.RunEffect();
  for (uint8_t i = 0; i < 5; i++) {
    EXPECT_EQ(manager.GetLed(i).r, 4 - i);
  }
}

// Render-loop flag-semantics tests (G3): RunEffect handles Off, Dim, Reversed
// and multi-strip global indexing centrally; these pin that contract using
// stub effects with exactly computable output.

// Distinct, non-degenerate values on every channel so Dim's /8 and Reversed's
// index flip both produce observable differences.
class FlagTestEffect : public Effect {
 public:
  FlagTestEffect() {}

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const {
    UNUSED(time_ms);
    UNUSED(strip);
    UNUSED(setEffectPacket);
    return CRGB(led_index * 40 + 17, 128, 255 - led_index * 30);
  }
};

// Encodes the strip it was called with, so global-index mapping mistakes
// (overlap, gap, wrong strip order) are visible per LED.
class StripCountEffect : public Effect {
 public:
  StripCountEffect() {}

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const {
    UNUSED(time_ms);
    UNUSED(setEffectPacket);
    return CRGB(led_index, strip.led_count, 0);
  }
};

// Builds the full render stack for a device with a single stub effect
// selected, mirroring how production wires LedManager to the state machine.
class RenderRig {
 public:
  RenderRig(const DeviceDescription &device, Effect *effect)
      : networkManager(&radio),
        state_machine(&networkManager),
        manager(device, &state_machine) {
    manager.ClearEffects();
    manager.PublicAddEffect(effect, 1);
    setEffect.writeSetEffect(0, 0, 0);
    state_machine.SetEffect(&setEffect);
  }

  FakeRadio radio;
  NetworkManager networkManager;
  RadioStateMachine state_machine;
  FakeLedManager manager;
  RadioPacket setEffect;
};

void ExpectLedEquals(FakeLedManager &manager, uint8_t global_index,
                     const CRGB &expected, const char *what) {
  const CRGB actual = manager.GetLed(global_index);
  EXPECT_EQ(actual.r, expected.r) << what << ": LED " << (int)global_index;
  EXPECT_EQ(actual.g, expected.g) << what << ": LED " << (int)global_index;
  EXPECT_EQ(actual.b, expected.b) << what << ": LED " << (int)global_index;
}

TEST(LedManager, runEffectAppliesOffDimReversedPerStrip) {
  DeviceDescription device = DeviceDescription(
      2000, {StripDescription(5, {}), StripDescription(5, {Off}),
             StripDescription(5, {Dim}), StripDescription(5, {Reversed})});
  RenderRig rig(device, new FlagTestEffect());
  rig.manager.RunEffect();

  // The unflagged strip (global 0-4) is the oracle the flagged strips are
  // asserted against.
  CRGB plain[5];
  for (uint8_t i = 0; i < 5; i++) {
    plain[i] = rig.manager.GetLed(i);
    const CRGB expected = FlagTestEffect().GetRGB(i, 0, device.strips[0],
                                                  rig.state_machine.GetSetEffect());
    ExpectLedEquals(rig.manager, i, expected, "plain strip");
  }

  for (uint8_t i = 0; i < 5; i++) {
    ExpectLedEquals(rig.manager, 5 + i, CRGB(0, 0, 0), "Off strip");
    ExpectLedEquals(
        rig.manager, 10 + i,
        CRGB(plain[i].r / 8, plain[i].g / 8, plain[i].b / 8), "Dim strip");
    ExpectLedEquals(rig.manager, 15 + i, plain[4 - i], "Reversed strip");
  }
}

TEST(LedManager, runEffectAppliesDimAndReversedTogether) {
  DeviceDescription device = DeviceDescription(
      2000, {StripDescription(5, {}), StripDescription(5, {Dim, Reversed})});
  RenderRig rig(device, new FlagTestEffect());
  rig.manager.RunEffect();

  for (uint8_t i = 0; i < 5; i++) {
    const CRGB plain = rig.manager.GetLed(4 - i);
    ExpectLedEquals(rig.manager, 5 + i,
                    CRGB(plain.r / 8, plain.g / 8, plain.b / 8),
                    "Dim+Reversed strip");
  }
}

TEST(LedManager, runEffectMapsStripsToContiguousGlobalRanges) {
  DeviceDescription device =
      DeviceDescription(2000, {StripDescription(3, {}), StripDescription(5, {}),
                               StripDescription(2, {})});
  ASSERT_EQ(device.GetLedCount(), 10);
  RenderRig rig(device, new StripCountEffect());
  rig.manager.RunEffect();

  // Each strip's LEDs must land at the correct contiguous global offsets, in
  // declaration order, with no overlap or gap: (strip-local index, strip led
  // count) recovered from every global LED.
  const struct {
    uint8_t local_index;
    uint8_t strip_size;
  } expected[10] = {{0, 3}, {1, 3}, {2, 3}, {0, 5}, {1, 5},
                    {2, 5}, {3, 5}, {4, 5}, {0, 2}, {1, 2}};
  for (uint8_t i = 0; i < 10; i++) {
    ExpectLedEquals(rig.manager, i,
                    CRGB(expected[i].local_index, expected[i].strip_size, 0),
                    "multi-strip mapping");
  }
}

TEST(LedManager, controlPacketOverridesEffectThenEffectResumes) {
  DeviceDescription device = DeviceDescription(
      2000, {StripDescription(5, {}), StripDescription(2, {Off})});
  RenderRig rig(device, new FlagTestEffect());
  rig.manager.RunEffect();
  const CRGB effect_led0 = rig.manager.GetLed(0);

  // A SET_CONTROL packet replaces the current effect with its solid color.
  // RadioStateMachine::SetEffect() parses its argument as SET_EFFECT, so
  // inject the control packet the way handleSlaveEvent/handleMasterEvent do
  // for a received SET_CONTROL packet.
  RadioPacket control;
  control.writeControl(0, CRGB(9, 87, 65));
  *rig.state_machine.GetSetEffect() = control;
  rig.manager.RunEffect();
  for (uint8_t i = 0; i < 5; i++) {
    ExpectLedEquals(rig.manager, i, CRGB(9, 87, 65), "control override");
  }
  for (uint8_t i = 5; i < 7; i++) {
    ExpectLedEquals(rig.manager, i, CRGB(0, 0, 0), "Off strip under control");
  }

  // Replacing the control packet with a SET_EFFECT resumes effect rendering.
  rig.state_machine.SetEffect(&rig.setEffect);
  rig.manager.RunEffect();
  ExpectLedEquals(rig.manager, 0, effect_led0, "effect resumed");
}