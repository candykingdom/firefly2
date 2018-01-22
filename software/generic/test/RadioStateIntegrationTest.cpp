#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeRadio.hpp"
#include "FakeNetwork.hpp"

TEST(Network, electsOneMaster) {
  FakeNetwork network;
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout);
  network.Tick();

  advanceMillis(1);
  network.Tick();

  advanceMillis(1);
  network.Tick();

  int numMasters = 0;
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    if (network.stateMachines[i]->GetCurrentState() == RadioState::Master) {
      numMasters++;
    }
  }

  EXPECT_EQ(numMasters, 1);
}
