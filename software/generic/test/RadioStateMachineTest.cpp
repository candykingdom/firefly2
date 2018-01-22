#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeRadio.hpp"

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

  setMillis(RadioStateMachine::kSlaveNoPacketTimeout + 1);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);
}

TEST(RadioStateMachine, sendsHeartbeats) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(RadioStateMachine::kSlaveNoPacketTimeout + 1);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);
  EXPECT_EQ(radio.getSentPacket(), nullptr);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval);
  stateMachine->Tick();
  RadioPacket *packet = radio.getSentPacket();
  EXPECT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval);
  stateMachine->Tick();
  packet = radio.getSentPacket();
  EXPECT_NE(packet, nullptr);
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

  radio.setReceivePacket(&packet);
  advanceMillis(RadioStateMachine::kSlaveNoPacketTimeout - 1);
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
  setMillis(RadioStateMachine::kSlaveNoPacketTimeout + 1);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  packet.packetId = 1;
  packet.type = CLAIM_MASTER;
  packet.dataLength = 0;
  radio.setReceivePacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Slave);
}
