#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeNetwork.hpp"
#include "FakeRadio.hpp"

int getNumMasters(FakeNetwork network) {
  int numMasters = 0;
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    if (network.stateMachines[i]->GetCurrentState() == RadioState::Master) {
      numMasters++;
    }
  }
  return numMasters;
}

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

TEST(Network, reelectsMasterIfMasterDisappears) {
  FakeNetwork network;
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 5);
  network.Tick();

  for (int i = 0; i < 10; i++) {
    advanceMillis(1);
    network.Tick();
  }

  EXPECT_EQ(network.stateMachines[0]->GetCurrentState(), RadioState::Master);
  EXPECT_EQ(network.stateMachines[1]->GetCurrentState(), RadioState::Slave);

  network.reinitNode(0);
  EXPECT_EQ(network.stateMachines[0]->GetCurrentState(), RadioState::Slave);

  // Run long enough that master election has happened and the network has
  // stabilized.
  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout + 10; i++) {
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

TEST(Network, stableOverTime) {
  // Using a fixed seed means that this test is deterministic
  srand(100);
  FakeNetwork network;

  // Advance long enough that master election has definitely happened and is
  // stable.
  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 2; i++) {
    advanceMillis(1);
    network.Tick();
  }

  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 5; i++) {
    advanceMillis(1);
    network.Tick();
    EXPECT_EQ(getNumMasters(network), 1);
  }
}

TEST(Network, stableWhenNodesDropOut) {
  // Using a fixed seed means that this test is deterministic
  srand(100);
  FakeNetwork network;

  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 2; i++) {
    advanceMillis(1);
    network.Tick();
  }

  for (int i = 0; i < 10; i++) {
    for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; i++) {
      advanceMillis(1);
      network.Tick();
      EXPECT_EQ(getNumMasters(network), 1);
    }
    network.reinitNode(rand() % FakeNetwork::kNumNodes);
    network.reinitNode(rand() % FakeNetwork::kNumNodes);
    for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 6 + 2; i++) {
      advanceMillis(1);
      network.Tick();
    }
    EXPECT_EQ(getNumMasters(network), 1);
  }
}
