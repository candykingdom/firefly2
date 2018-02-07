#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeNetwork.hpp"
#include "FakeRadio.hpp"

TEST(Network, electsOneMaster) {
  FakeNetwork network;
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 5);
  network.Tick();

  for (int i = 0; i < 10; i++) {
    advanceMillis(1);
    network.Tick();
  }

  int numMasters = 0;
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    if (network.stateMachines[i]->GetCurrentState() == RadioState::Master) {
      numMasters++;
    }
  }

  EXPECT_EQ(numMasters, 1);
}
