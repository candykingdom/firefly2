#include <DeviceDescription.hpp>
#include <LedManager.hpp>
#include <algorithm>
#include <vector>

#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

// Make sure that all of the effects can run for a while without crashing
void runEffectsTest(DeviceDescription* device, uint32_t maxTime) {
  FakeRadio radio;
  NetworkManager* networkManager = new NetworkManager(&radio);
  RadioStateMachine* state_machine = new RadioStateMachine(networkManager);
  FakeLedManager* manager = new FakeLedManager(device, state_machine);
  RadioPacket packet;
  std::vector<Effect*> ran_effects;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    packet.writeSetEffect(i, 0, 0);
    state_machine->SetEffect(&packet);
    Effect* effect = manager->GetCurrentEffect();

    // Effects are duplicated, make sure we haven't already tested this one.
    if (std::find(ran_effects.begin(), ran_effects.end(), effect) !=
        ran_effects.end()) {
      continue;
    }
    ran_effects.push_back(effect);

    for (uint32_t time = 0; time < maxTime; time++) {
      manager->RunEffect();
    }
  }
  delete manager;
  delete state_machine;
}

TEST(Effects, oneLed) {
  StripDescription* one_led_strip = new StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {one_led_strip});
  runEffectsTest(&device, 60 * 1000);
  delete one_led_strip;
}

TEST(Effects, hundredLeds) {
  StripDescription* hundred_led_strip = new StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {hundred_led_strip});
  runEffectsTest(&device, 60 * 1000);
  delete hundred_led_strip;
}

TEST(Effects, allLedValues) {
  StripDescription* one_led_strip = new StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {one_led_strip});
  for (uint16_t num_leds = 0; num_leds < 256; num_leds++) {
    runEffectsTest(&device, 5 * 1000);
  }
  delete one_led_strip;
}

TEST(Effects, multipleStrips) {
  StripDescription* twenty_led_strip = new StripDescription(20, {});
  StripDescription* ten_led_strip = new StripDescription(10, {Tiny});
  StripDescription* twelve_led_strip = new StripDescription(12, {Circular});
  DeviceDescription device = DeviceDescription(
      2000, {twenty_led_strip, ten_led_strip, twelve_led_strip});
  EXPECT_EQ(device.GetLedCount(), 42);
  runEffectsTest(&device, 60 * 1000);
  delete twelve_led_strip;
  delete ten_led_strip;
  delete twenty_led_strip;
}
