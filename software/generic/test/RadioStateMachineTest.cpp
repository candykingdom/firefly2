#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeRadio.hpp"

const uint16_t kMaxSlaveTimeout = RadioStateMachine::kSlaveNoPacketTimeout +
                                  RadioStateMachine::kSlaveNoPacketRandom + 1;

TEST(RadioStateMachine, initializes) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, becomesMasterAfterTimeout) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);

  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);
}

TEST(RadioStateMachine, sendsHeartbeats) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);
  RadioPacket *packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval + 1);
  stateMachine->Tick();
  packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval + 1);
  stateMachine->Tick();
  packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);
}

TEST(RadioStateMachine, doesntBecomeMasterIfReceivesPackets) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  RadioPacket packet;
  packet.packetId = 1;
  packet.type = HEARTBEAT;
  packet.dataLength = 0;

  radio.setReceivedPacket(&packet);
  advanceMillis(kMaxSlaveTimeout);
  advanceMillis(2);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, respectsClaimMaster) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  packet.packetId = 1;
  packet.type = CLAIM_MASTER;
  packet.dataLength = 0;
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, doesElectionAndBecomesSlave) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  // The random ID for master election is in the range [2, 0xFFFF)
  packet.packetId = 0xFFFF;
  packet.type = HEARTBEAT;
  packet.dataLength = 0;
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, doesElectionAndBecomesMaster) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  // The random ID for master election is in the range [2, 0xFFFF)
  packet.packetId = 1;
  packet.type = HEARTBEAT;
  packet.dataLength = 0;
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket *receivedPacket = radio.getSentPacket();
  EXPECT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, CLAIM_MASTER);
}
