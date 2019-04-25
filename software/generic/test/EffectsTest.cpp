#include "gtest/gtest.h"

#include "../LedManager.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"

// Make sure that all of the effects can run for a while without crashing
void runEffectsTest(uint8_t numLeds) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine =
      new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(numLeds, stateMachine);
  RadioPacket packet;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    packet.writeSetEffect(i, 0, 0);
    stateMachine->SetEffect(&packet);
    for (uint32_t time = 0; time < 60 * 1000; time++) {
      manager->RunEffect();
    }
  }
}

TEST(Effects, oneLed) {
  runEffectsTest(1);
}

TEST(Effects, hundredLeds) {
  runEffectsTest(100);
}
