#include "gtest/gtest.h"

#include "../LedManager.hpp"
#include "FakeLedManager.hpp"

// Make sure that all of the effects can run for a while without crashing
void runEffectsTest(FakeLedManager *manager) {
  RadioPacket packet;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    manager->SetEffect(i);
    packet.writeSetEffect(i, 0, 0);
    for (uint32_t time = 0; time < 60 * 1000; time++) {
      manager->RunEffect(time, &packet);
    }
  }
}

TEST(Effects, oneLed) {
  FakeLedManager *manager = new FakeLedManager(1);
  runEffectsTest(manager);
}

TEST(Effects, hundredLeds) {
  FakeLedManager *manager = new FakeLedManager(100);
  runEffectsTest(manager);
}
