#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeNetwork.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"
#include "RadioIntegrationTest.hpp"

class RadioStateIntegrationTest : public RadioIntegrationTest {};

TEST_F(RadioStateIntegrationTest, electsOneMaster) {
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 5);
  network.Tick();

  runTicks(RadioStateMachine::kSlaveNoPacketRandom + 10);
  EXPECT_EQ(getNumMasters(), 1);
}

TEST_F(RadioStateIntegrationTest, relectsMasterIfMasterDisappears) {
  network.Tick();
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 5);
  network.Tick();

  // Run long enough that master election has happened and the network has
  // stabilized.
  runTicks(RadioStateMachine::kSlaveNoPacketRandom + 10);

  EXPECT_EQ(getNumMasters(), 1);
  resetMaster();

  runTicks(RadioStateMachine::kSlaveNoPacketTimeout +
                        RadioStateMachine::kSlaveNoPacketRandom + 10);

  EXPECT_EQ(getNumMasters(), 1);
}

TEST_F(RadioStateIntegrationTest, stableOverTime) {
  runTicks(RadioStateMachine::kSlaveNoPacketTimeout * 2);

  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 5; i++) {
    advanceMillis(1);
    network.Tick();
    EXPECT_EQ(getNumMasters(), 1);
  }
}

TEST_F(RadioStateIntegrationTest, stableWhenNodesDropOut) {
  runTicks(RadioStateMachine::kSlaveNoPacketTimeout +
                        RadioStateMachine::kSlaveNoPacketRandom + 10);
  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; i++) {
    advanceMillis(1);
    network.Tick();
    ASSERT_EQ(getNumMasters(), 1);
  }

  for (int i = 0; i < 10; i++) {
    // "Bad" ticks are ones where there isn't exactly one master.
    int currentBadTicks = 0;
    int maxBadTicks = 0;
    int totalBadTicks = 0;
    bool badPrev = false;

    for (int j = 0; j < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; j++) {
      advanceMillis(1);
      network.Tick();

      int numMasters = getNumMasters();
      if (numMasters != 1) {
        totalBadTicks++;
        if (badPrev) {
          currentBadTicks++;
          if (currentBadTicks > maxBadTicks) {
            maxBadTicks = currentBadTicks;
          }
        }
        badPrev = true;
      } else {
        badPrev = false;
      }
    }

    // After enough time, the network should be totally stable
    for (int j = 0; j < 100; j++) {
      advanceMillis(1);
      network.Tick();
      EXPECT_EQ(getNumMasters(), 1);
    }

    EXPECT_LT(maxBadTicks, 10) << "On iteration " << i;
    EXPECT_LT(totalBadTicks, 10) << "On iteration " << i;
    network.reinitNode(rand() % FakeNetwork::kNumNodes);
    network.reinitNode(rand() % FakeNetwork::kNumNodes);

    runTicks(RadioStateMachine::kSlaveNoPacketTimeout * 2);
    EXPECT_EQ(getNumMasters(), 1);
  }
}

TEST_F(RadioStateIntegrationTest, stableWithMildPacketLoss) {
  // 1% packet loss
  network.setPacketLoss(100);

  runTicks(RadioStateMachine::kSlaveNoPacketTimeout * 2);

  int currentBadTicks = 0;
  int maxBadTicks = 0;
  int totalBadTicks = 0;
  bool badPrev = false;
  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 10; i++) {
    advanceMillis(1);
    network.Tick();

    int numMasters = getNumMasters();
    if (numMasters != 1) {
      totalBadTicks++;
      if (badPrev) {
        currentBadTicks++;
        if (currentBadTicks > maxBadTicks) {
          maxBadTicks = currentBadTicks;
        }
      }
      badPrev = true;
    } else {
      badPrev = false;
    }
  }

  EXPECT_LT(maxBadTicks, 50);
  EXPECT_LT(totalBadTicks, 100);
}

TEST_F(RadioStateIntegrationTest, stableWithMildPacketLossAndNodesDropping) {
  // 1% packet loss
  network.setPacketLoss(100);

  runTicks(RadioStateMachine::kSlaveNoPacketTimeout +
                        RadioStateMachine::kSlaveNoPacketRandom + 10);
  for (int i = 0; i < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; i++) {
    advanceMillis(1);
    network.Tick();
    ASSERT_EQ(getNumMasters(), 1);
  }

  for (int i = 0; i < 10; i++) {
    int currentBadTicks = 0;
    int maxBadTicks = 0;
    int totalBadTicks = 0;
    bool badPrev = false;

    for (int j = 0; j < RadioStateMachine::kSlaveNoPacketTimeout * 2 + 2; j++) {
      advanceMillis(1);
      network.Tick();

      int numMasters = getNumMasters();
      if (numMasters != 1) {
        totalBadTicks++;
        if (badPrev) {
          currentBadTicks++;
          if (currentBadTicks > maxBadTicks) {
            maxBadTicks = currentBadTicks;
          }
        }
        badPrev = true;
      } else {
        badPrev = false;
      }
    }

    EXPECT_LT(maxBadTicks, 500) << "On iteration " << i;
    EXPECT_LT(totalBadTicks, 1000) << "On iteration " << i;
    network.reinitNode(rand() % FakeNetwork::kNumNodes);
    network.reinitNode(rand() % FakeNetwork::kNumNodes);

    runTicks(RadioStateMachine::kSlaveNoPacketTimeout * 2);
  }
}

TEST_F(RadioStateIntegrationTest, setEffectIndex) {
  runTicks(1);
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    EXPECT_EQ(network.stateMachines[i]->GetEffectIndex(), 0)
        << "Node " << i << " has wrong effect index";
  }

  // Note: multiplier of 3 here is a magic number, so that the random SetEffect
  // is 1 (i.e. different from the default). Since we choose a fixed random
  // seed, this is stable.
  runTicks(RadioStateMachine::kChangeEffectInterval * 3);
  // All nodes should have the same, non-zero (i.e. default) effect index
  uint8_t expectedEffectIndex = network.stateMachines[0]->GetEffectIndex();
  EXPECT_NE(expectedEffectIndex, 1);
  for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
    EXPECT_EQ(network.stateMachines[i]->GetEffectIndex(), expectedEffectIndex)
        << "Node " << i << " has wrong effect index";
  }
}

TEST_F(RadioStateIntegrationTest, changesEffectWhenMasterNotStable) {
  runTicks(RadioStateMachine::kChangeEffectInterval - 10);

  uint8_t originalEffect = network.stateMachines[0]->GetEffectIndex();

  resetMaster();
  runTicks(100);

  uint8_t newEffect = network.stateMachines[0]->GetEffectIndex();
  EXPECT_NE(originalEffect, newEffect);
}

TEST_F(RadioStateIntegrationTest, invalidPacket) {
  runTicks(RadioStateMachine::kSlaveNoPacketTimeout + RadioStateMachine::kMasterHeartbeatInterval * 3);
  // Invalid type
  RadioPacket packet = {packetId: 1, type: (PacketType) 5, dataLength: 2, data: {5, 6}};
  network.TransmitPacket(packet);
  runTicks(100);
}
