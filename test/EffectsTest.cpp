#include <DeviceDescription.hpp>
#include <LedManager.hpp>
#include <algorithm>
#include <vector>

#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

class EffectsTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  void runEffectsTest(uint32_t maxTime, uint32_t time_step = 1);

  FakeRadio radio;
  NetworkManager networkManager = NetworkManager(&radio);
  RadioStateMachine state_machine = RadioStateMachine(&networkManager);
  FakeLedManager* manager = nullptr;
};

// Make sure that all of the effects can run for a while without crashing
void EffectsTest::runEffectsTest(uint32_t maxTime, uint32_t time_step) {
  RadioPacket packet;
  std::vector<Effect*> ran_effects;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    packet.writeSetEffect(i, 0, 0);
    state_machine.SetEffect(&packet);
    Effect* effect = manager->GetCurrentEffect();

    // Effects are duplicated, make sure we haven't already tested this one.
    if (std::find(ran_effects.begin(), ran_effects.end(), effect) !=
        ran_effects.end()) {
      continue;
    }
    ran_effects.push_back(effect);

    for (uint32_t time = 0; time < maxTime; time += time_step) {
      setMillis(time);
      manager->RunEffect();
    }
  }
}

TEST_F(EffectsTest, oneLed) {
  StripDescription one_led_strip(1, {});
  DeviceDescription device = DeviceDescription(2000, {one_led_strip});
  manager = new FakeLedManager(device, &state_machine);
  runEffectsTest(60 * 1000);
  delete manager;
}

TEST_F(EffectsTest, hundredLeds) {
  StripDescription hundred_led_strip(100, {});
  DeviceDescription device = DeviceDescription(2000, {hundred_led_strip});
  manager = new FakeLedManager(device, &state_machine);
  runEffectsTest(60 * 1000);
  delete manager;
}

TEST_F(EffectsTest, allLedValues) {
  for (uint16_t num_leds = 0; num_leds < 256; num_leds++) {
    StripDescription strip(num_leds, {});
    DeviceDescription device = DeviceDescription(2000, {strip});
    manager = new FakeLedManager(device, &state_machine);
    ASSERT_NE(manager, nullptr) << "num_leds: " << num_leds;
    runEffectsTest(10 * 1000, 100);
    delete manager;
  }
}

TEST_F(EffectsTest, multipleStrips) {
  StripDescription twenty_led_strip(20, {});
  StripDescription ten_led_strip(10, {Tiny});
  StripDescription twelve_led_strip(12, {Circular});
  DeviceDescription device = DeviceDescription(
      2000, {twenty_led_strip, ten_led_strip, twelve_led_strip});
  EXPECT_EQ(device.GetLedCount(), 42);
  manager = new FakeLedManager(device, &state_machine);
  runEffectsTest(60 * 1000);
  delete manager;
}

TEST_F(EffectsTest, allColorPalettes) {
  StripDescription strip(20, {});
  DeviceDescription device = DeviceDescription(2000, {strip});
  manager = new FakeLedManager(device, &state_machine);

  RadioPacket packet;
  std::vector<Effect*> ran_effects;
  for (uint8_t effect_index = 0; effect_index < manager->GetNumEffects();
       effect_index++) {
    for (uint8_t palette_index = 0; palette_index < Effect::palettes().size();
         palette_index++) {
      packet.writeSetEffect(effect_index, /*delay=*/0, palette_index);
      state_machine.SetEffect(&packet);
      Effect* effect = manager->GetCurrentEffect();

      // Effects are duplicated, make sure we haven't already tested this one.
      if (std::find(ran_effects.begin(), ran_effects.end(), effect) !=
          ran_effects.end()) {
        continue;
      }
      ran_effects.push_back(effect);

      for (uint32_t time = 0; time < 60 * 1000; time += 10) {
        setMillis(time);
        manager->RunEffect();
      }
    }
  }
  delete manager;
}
