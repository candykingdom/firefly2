#ifndef __RADIO_INTEGRATION_TEST_H__
#define __RADIO_INTEGRATION_TEST_H__

#include "FakeNetwork.hpp"
#include "gtest/gtest.h"

class RadioIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Using a fixed seed makes tests deterministic
    srand(100);
  }

  int getNumMasters() {
    int num_masters = 0;
    for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
      if (network.stateMachines[i]->GetCurrentState() == RadioState::Master) {
        num_masters++;
      }
    }
    return num_masters;
  }

  void runTicks(int ticks) {
    for (int i = 0; i < ticks; i++) {
      advanceMillis(1);
      network.Tick();
    }
  }

  void resetMaster() {
    for (int i = 0; i < FakeNetwork::kNumNodes; i++) {
      if (network.stateMachines[0]->GetCurrentState() == RadioState::Master) {
        network.reinitNode(i);
        EXPECT_EQ(network.stateMachines[i]->GetCurrentState(),
                  RadioState::Slave);
      }
    }
  }

  FakeNetwork network;
};

#endif  // __RADIO_INTEGRATION_TEST_H__