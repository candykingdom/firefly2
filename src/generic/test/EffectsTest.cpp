#include <DeviceDescription.hpp>
#include <LedManager.hpp>
#include <algorithm>
#include <vector>

#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

// Make sure that all of the effects can run for a while without crashing
void runEffectsTest(DeviceDescription *device, uint32_t maxTime) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(device, state_machine);
  RadioPacket packet;
  std::vector<Effect *> ran_effects;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    packet.writeSetEffect(i, 0, 0);
    state_machine->SetEffect(&packet);
    Effect *effect = manager->GetCurrentEffect();

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
}

TEST(Effects, oneLed) {
  DeviceDescription device =
      DeviceDescription(2000, {new StripDescription(1, {})});
  runEffectsTest(&device, 60 * 1000);
}

TEST(Effects, hundredLeds) {
  DeviceDescription device =
      DeviceDescription(2000, {new StripDescription(100, {})});
  runEffectsTest(&device, 60 * 1000);
}

TEST(Effects, allLedValues) {
  DeviceDescription device =
      DeviceDescription(2000, {new StripDescription(1, {})});
  for (uint16_t num_leds = 0; num_leds < 256; num_leds++) {
    runEffectsTest(&device, 5 * 1000);
  }
}

TEST(Effects, multipleStrips) {
  DeviceDescription device = DeviceDescription(
      2000, {new StripDescription(20, {}), new StripDescription(10, {Tiny}),
             new StripDescription(12, {Circular})});
  EXPECT_EQ(device.GetLedCount(), 42);
  runEffectsTest(&device, 60 * 1000);
}
