#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeNetwork.hpp"
#include "FakeRadio.hpp"

int getNumMasters(FakeNetwork &network) {
  int numMasters = 0;
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    if (network.stateMachines[i]->GetCurrentState() == RadioState::Master) {
      numMasters++;
    }
  }
  return numMasters;
}

void runTicks(FakeNetwork &network, int ticks) {
  for (int i = 0; i < ticks; i++) {
    advanceMillis(1);
    network.Tick();
  }
}

TEST(Network, electsOneMaster) {
  FakeNetwork network;
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 5);
  network.Tick();

  runTicks(network, RadioStateMachine::kSlaveNoPacketRandom + 10);
  EXPECT_EQ(getNumMasters(network), 1);
}

TEST(Network, relectsMasterIfMasterDisappears) {
  FakeNetwork network;
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 5);
  network.Tick();

  // Run long enough that master election has happened and the network has
  // stabilized.
  runTicks(network, RadioStateMachine::kSlaveNoPacketRandom + 10);

  EXPECT_EQ(getNumMasters(network), 1);
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    if (network.stateMachines[0]->GetCurrentState() == RadioState::Master) {
      network.reinitNode(i);
      EXPECT_EQ(network.stateMachines[i]->GetCurrentState(), RadioState::Slave);
    }
  }

  runTicks(network,
           RadioStateMachine::kSlaveNoPacketTimeout +
               RadioStateMachine::kSlaveNoPacketRandom + 10);

  EXPECT_EQ(getNumMasters(network), 1);
}

TEST(Network, stableOverTime) {
  // Using a fixed seed means that this test is deterministic
  FakeNetwork network;

  runTicks(network, RadioStateMachine::kSlaveNoPacketTimeout * 2);

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

  runTicks(network,
           RadioStateMachine::kSlaveNoPacketTimeout +
               RadioStateMachine::kSlaveNoPacketRandom + 10);
  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; i++) {
    advanceMillis(1);
    network.Tick();
    ASSERT_EQ(getNumMasters(network), 1);
  }

  for (int i = 0; i < 10; i++) {
    int currentTicksWithMoreThanOneMaster = 0;
    int maxTicksWithMoreThanOneMaster = 0;
    int totalTicksWithMoreThanOneMaster = 0;
    bool moreThanOneMasterPrev = false;

    for (int j = 0; j < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; j++) {
      advanceMillis(1);
      network.Tick();

      int numMasters = getNumMasters(network);
      if (numMasters != 1) {
        totalTicksWithMoreThanOneMaster++;
        if (moreThanOneMasterPrev) {
          currentTicksWithMoreThanOneMaster++;
          if (currentTicksWithMoreThanOneMaster >
              maxTicksWithMoreThanOneMaster) {
            maxTicksWithMoreThanOneMaster = currentTicksWithMoreThanOneMaster;
          }
        }
        moreThanOneMasterPrev = true;
      } else {
        moreThanOneMasterPrev = false;
      }
    }

    // After enough time, the network should be totally stable
    for (int j = 0; j < 100; j++) {
      advanceMillis(1);
      network.Tick();
      EXPECT_EQ(getNumMasters(network), 1);
    }

    EXPECT_LT(maxTicksWithMoreThanOneMaster, 10) << "On iteration " << i;
    EXPECT_LT(totalTicksWithMoreThanOneMaster, 10) << "On iteration " << i;
    network.reinitNode(rand() % FakeNetwork::kNumNodes);
    network.reinitNode(rand() % FakeNetwork::kNumNodes);

    runTicks(network, RadioStateMachine::kSlaveNoPacketTimeout * 2);
    EXPECT_EQ(getNumMasters(network), 1);
  }
}
