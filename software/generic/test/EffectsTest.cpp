#include <algorithm>
#include <vector>

#include "../LedManager.hpp"
#include "DeviceDescription.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

// Make sure that all of the effects can run for a while without crashing
void runEffectsTest(DeviceDescription *device, uint32_t maxTime) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(device, stateMachine);
  RadioPacket packet;
  std::vector<Effect *> ran_effects;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    packet.writeSetEffect(i, 0, 0);
    stateMachine->SetEffect(&packet);
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
  LinearDescription device = LinearDescription(1, DeviceType::Wearable);
  runEffectsTest(&device, 60 * 1000);
}

TEST(Effects, hundredLeds) {
  LinearDescription device = LinearDescription(1, DeviceType::Wearable);
  runEffectsTest(&device, 60 * 1000);
}

TEST(Effects, allLedValues) {
  LinearDescription device = LinearDescription(1, DeviceType::Wearable);
  for (uint16_t numLeds = 0; numLeds < 256; numLeds++) {
    runEffectsTest(&device, 5 * 1000);
  }
}
