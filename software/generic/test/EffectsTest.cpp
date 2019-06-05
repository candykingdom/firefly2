#include "gtest/gtest.h"

#include "../LedManager.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"

#include <algorithm>
#include <vector>

// Make sure that all of the effects can run for a while without crashing
void runEffectsTest(uint8_t numLeds, uint32_t maxTime) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(numLeds, stateMachine);
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

TEST(Effects, oneLed) { runEffectsTest(1, 60 * 1000); }

TEST(Effects, hundredLeds) { runEffectsTest(100, 60 * 1000); }

TEST(Effects, allLedValues) {
  for (uint16_t numLeds = 0; numLeds < 256; numLeds++) {
    runEffectsTest(1, 5 * 1000);
  }
}
