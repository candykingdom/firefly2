#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

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
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  EXPECT_EQ(radio.getSentPacket(), nullptr);
  state_machine->RadioTick();
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

TEST(RadioStateMachine, initializes) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, becomesMasterAfterTimeout) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);
}

TEST(RadioStateMachine, sendsHeartbeats) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);
  RadioPacket *packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval + 1);
  state_machine->RadioTick();
  packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval + 1);
  state_machine->RadioTick();
  packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);

  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

TEST(RadioStateMachine, doesntBecomeMasterIfReceivesPackets) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  RadioPacket packet;
  packet.packet_id = 1;
  packet.type = HEARTBEAT;
  packet.dataLength = 0;

  radio.setReceivedPacket(&packet);
  advanceMillis(kMaxSlaveTimeout);
  advanceMillis(2);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, respectsClaimMaster) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  packet.packet_id = 1;
  packet.type = CLAIM_MASTER;
  packet.dataLength = 0;
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, doesElectionAndBecomesSlave) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  // The random ID for master election is in the range [2, 0xFFFF)
  packet.packet_id = 0xFFFF;
  packet.type = HEARTBEAT;
  packet.dataLength = 0;
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
}

TEST(RadioStateMachine, doesElectionAndBecomesMaster) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket packet;
  // The random ID for master election is in the range [2, 0xFFFF)
  packet.packet_id = 1;
  packet.type = HEARTBEAT;
  packet.dataLength = 0;
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket *received_packet = radio.getSentPacket();
  EXPECT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, CLAIM_MASTER);
}

TEST(RadioStateMachine, returnNetworkMillisForNoOffset) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  setMillis(0);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 0);

  setMillis(12345);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 12345);
}

TEST(RadioStateMachine, slaveGetsTimeFromNetwork) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  RadioPacket packet;
  packet.writeHeartbeat(10000);
  // The random ID for master election is in the range [2, 0xFFFF)
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetNetworkMillis(), 10000);

  setMillis(2000000);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 2010000);
}

TEST(RadioStateMachine, slaveGetsTimeFromNetwork_negativeOffset) {
  setMillis(10000);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  RadioPacket packet;
  packet.writeHeartbeat(0);
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetNetworkMillis(), 0);

  setMillis(2000000);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 1990000);
}

TEST(RadioStateMachine, masterSendsSetEffect) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  state_machine->SetNumEffects(255);
  state_machine->SetNumPalettes(255);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket *received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kChangeEffectInterval + 1);
  // First we'll get a heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);

  // Then the set effect
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, SET_EFFECT);
  EXPECT_NE(received_packet->readEffectIndexFromSetEffect(), 0);
  EXPECT_EQ(received_packet->readDelayFromSetEffect(), 0);
  EXPECT_NE(received_packet->readPaletteIndexFromSetEffect(), 0);

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kChangeEffectInterval +
            RadioStateMachine::kBroadcastEffectInterval + 1);
  // Another heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);

  // And now the same SetEffect
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, SET_EFFECT);
  EXPECT_NE(received_packet->readEffectIndexFromSetEffect(), 0);
  EXPECT_EQ(received_packet->readDelayFromSetEffect(), 0);
  EXPECT_NE(received_packet->readPaletteIndexFromSetEffect(), 0);
}

TEST(RadioStateMachine, masterRespectsSetEffectDelay) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  state_machine->SetNumEffects(255);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket *received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);

  RadioPacket setEffectPacket;
  setEffectPacket.writeSetEffect(2, 120, 0);
  radio.setReceivedPacket(&setEffectPacket);
  state_machine->RadioTick();
  state_machine->RadioTick();

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kChangeEffectInterval + 1);
  // Heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);

  // Re-broadcasts the same set effect packet
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  received_packet->packet_id = setEffectPacket.packet_id;
  ASSERT_EQ(*received_packet, setEffectPacket);

  setMillis(kMaxSlaveTimeout + 120 * 1000 + 1);
  // First we'll get a heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);

  // Then the set effect
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, SET_EFFECT);
  EXPECT_NE(received_packet->readEffectIndexFromSetEffect(), 0);
}

TEST(RadioStateMachine, slaveReturnsEffectIndexFromNetwork) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  EXPECT_EQ(state_machine->GetEffectIndex(), 0);

  RadioPacket packet;
  packet.writeSetEffect(42, 0, 0);
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetEffectIndex(), 42);
}

TEST(RadioStateMachine, slaveReturnsSetEffectPacketFromNetwork) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  RadioPacket *setEffect = state_machine->GetSetEffect();
  // Default SetEffect packet
  EXPECT_EQ(setEffect->readEffectIndexFromSetEffect(), 1);
  EXPECT_EQ(setEffect->readDelayFromSetEffect(), 0);
  EXPECT_EQ(setEffect->readPaletteIndexFromSetEffect(), 0);

  RadioPacket packet;
  packet.writeSetEffect(42, 0, 0);
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(*state_machine->GetSetEffect(), packet);
}

TEST(RadioStateMachine, SetEffect_Delay) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  RadioPacket *defaultSetEffect = state_machine->GetSetEffect();

  RadioPacket setEffect;
  setEffect.writeSetEffect(1, /* delay= */ 5, 2);
  state_machine->SetEffect(&setEffect);
  state_machine->RadioTick();

  expectPacketsEqual(state_machine->GetSetEffect(), &setEffect);
  expectPacketsEqual(radio.getSentPacket(), &setEffect);

  setMillis(5 * 1000 + 1);
  state_machine->RadioTick();
  expectPacketsEqual(state_machine->GetSetEffect(), defaultSetEffect);
}

TEST(RadioStateMachine, SetEffect_NoDelay) {
  setMillis(0);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);

  RadioPacket *defaultSetEffect = state_machine->GetSetEffect();

  RadioPacket setEffect;
  setEffect.writeSetEffect(1, /* delay= */ 0, 2);
  state_machine->SetEffect(&setEffect);
  state_machine->RadioTick();

  expectPacketsEqual(state_machine->GetSetEffect(), &setEffect);
  expectPacketsEqual(radio.getSentPacket(), &setEffect);

  setMillis(RadioStateMachine::kChangeEffectInterval + 1);
  state_machine->RadioTick();
  expectPacketsEqual(state_machine->GetSetEffect(), defaultSetEffect);
}
