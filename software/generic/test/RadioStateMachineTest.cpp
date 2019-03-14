#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeRadio.hpp"

const uint16_t kMaxSlaveTimeout = RadioStateMachine::kSlaveNoPacketTimeout +
                                  RadioStateMachine::kSlaveNoPacketRandom + 1;

void expectPacketsEqual(RadioPacket *const p1, RadioPacket *const p2) {
  EXPECT_EQ(p1->dataLength, p2->dataLength);
  for (uint8_t i = 0; i < p1->dataLength; i++) {
    EXPECT_EQ(p1->data[i], p2->data[i]) << "byte " << i;
  }
}

TEST(RadioStateMachine, fakeWorks) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  EXPECT_EQ(radio.getSentPacket(), nullptr);
  stateMachine->Tick();
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

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

  EXPECT_EQ(radio.getSentPacket(), nullptr);
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

TEST(RadioStateMachine, returnNetworkMillisForNoOffset) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(0);
  EXPECT_EQ(stateMachine->GetNetworkMillis(), 0);

  setMillis(12345);
  EXPECT_EQ(stateMachine->GetNetworkMillis(), 12345);
}

TEST(RadioStateMachine, slaveGetsTimeFromNetwork) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  RadioPacket packet;
  packet.writeHeartbeat(10000);
  // The random ID for master election is in the range [2, 0xFFFF)
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetNetworkMillis(), 10000);

  setMillis(2000000);
  EXPECT_EQ(stateMachine->GetNetworkMillis(), 2010000);
}

TEST(RadioStateMachine, slaveGetsTimeFromNetwork_negativeOffset) {
  setMillis(10000);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  RadioPacket packet;
  packet.writeHeartbeat(0);
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetNetworkMillis(), 0);

  setMillis(2000000);
  EXPECT_EQ(stateMachine->GetNetworkMillis(), 1990000);
}

TEST(RadioStateMachine, masterSendsSetEffect) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket *receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, HEARTBEAT);

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kSetEffectInterval + 1);
  // First we'll get a heartbeat
  stateMachine->Tick();
  receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, HEARTBEAT);

  // Then the set effect
  stateMachine->Tick();
  receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, SET_EFFECT);
  EXPECT_EQ(receivedPacket->readEffectIndexFromSetEffect(), 1);
  EXPECT_EQ(receivedPacket->readDelayFromSetEffect(), 0);
}

TEST(RadioStateMachine, masterRespectsSetEffectDelay) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetCurrentState(), RadioState::Master);

  RadioPacket *receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, HEARTBEAT);

  RadioPacket setEffectPacket;
  setEffectPacket.writeSetEffect(2, 30, 0);
  radio.setReceivedPacket(&setEffectPacket);
  stateMachine->Tick();
  stateMachine->Tick();

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kSetEffectInterval + 1);
  // Heartbeat
  stateMachine->Tick();
  receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, HEARTBEAT);

  // No more packets yet
  stateMachine->Tick();
  receivedPacket = radio.getSentPacket();
  ASSERT_EQ(receivedPacket, nullptr);

  setMillis(kMaxSlaveTimeout + 30 * 1000 + 1);
  // First we'll get a heartbeat
  stateMachine->Tick();
  receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, HEARTBEAT);

  // Then the set effect
  stateMachine->Tick();
  receivedPacket = radio.getSentPacket();
  ASSERT_NE(receivedPacket, nullptr);
  EXPECT_EQ(receivedPacket->type, SET_EFFECT);
  EXPECT_EQ(receivedPacket->readEffectIndexFromSetEffect(), 1);
}

TEST(RadioStateMachine, slaveReturnsEffectIndexFromNetwork) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  EXPECT_EQ(stateMachine->GetEffectIndex(), 0);

  RadioPacket packet;
  packet.writeSetEffect(42, 0, 0);
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(stateMachine->GetEffectIndex(), 42);
}

TEST(RadioStateMachine, slaveReturnsSetEffectPacketFromNetwork) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  RadioPacket *setEffect = stateMachine->GetSetEffect();
  // Default SetEffect packet
  EXPECT_EQ(setEffect->readEffectIndexFromSetEffect(), 1);
  EXPECT_EQ(setEffect->readDelayFromSetEffect(), 0);
  EXPECT_EQ(setEffect->readPaletteIndexFromSetEffect(), 0);

  RadioPacket packet;
  packet.writeSetEffect(42, 0, 0);
  radio.setReceivedPacket(&packet);
  stateMachine->Tick();
  EXPECT_EQ(*stateMachine->GetSetEffect(), packet);
}

TEST(RadioStateMachine, SetEffect_Delay) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  RadioPacket *defaultSetEffect = stateMachine->GetSetEffect();

  RadioPacket setEffect;
  setEffect.writeSetEffect(1, /* delay= */ 5, 2);
  stateMachine->SetEffect(&setEffect);
  stateMachine->Tick();

  expectPacketsEqual(stateMachine->GetSetEffect(), &setEffect);
  expectPacketsEqual(radio.getSentPacket(), &setEffect);

  setMillis(5 * 1000 + 1);
  stateMachine->Tick();
  expectPacketsEqual(stateMachine->GetSetEffect(), defaultSetEffect);
}

TEST(RadioStateMachine, SetEffect_NoDelay) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);

  RadioPacket *defaultSetEffect = stateMachine->GetSetEffect();

  RadioPacket setEffect;
  setEffect.writeSetEffect(1, /* delay= */ 0, 2);
  stateMachine->SetEffect(&setEffect);
  stateMachine->Tick();

  expectPacketsEqual(stateMachine->GetSetEffect(), &setEffect);
  expectPacketsEqual(radio.getSentPacket(), &setEffect);

  setMillis(RadioStateMachine::kSetEffectInterval + 1);
  stateMachine->Tick();
  expectPacketsEqual(stateMachine->GetSetEffect(), defaultSetEffect);
}
