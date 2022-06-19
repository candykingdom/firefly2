#include <Radio.hpp>

#include "../NetworkManager.hpp"
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

class RadioStateMachineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    setMillis(0);
    network_manager = new NetworkManager(&radio);
    state_machine = new RadioStateMachine(network_manager);
  }

  void TearDown() override { delete state_machine; }

  FakeRadio radio;
  NetworkManager *network_manager;
  RadioStateMachine *state_machine;
};

TEST_F(RadioStateMachineTest, fakeWorks) {
  EXPECT_EQ(radio.getSentPacket(), nullptr);
  state_machine->RadioTick();
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

TEST_F(RadioStateMachineTest, initializes) {
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
}

TEST_F(RadioStateMachineTest, becomesMasterAfterTimeout) {
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Slave);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);
}

TEST_F(RadioStateMachineTest, sendsHeartbeats) {
  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);
  RadioPacket *packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);
  delete packet;

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval + 1);
  state_machine->RadioTick();
  packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);
  delete packet;

  advanceMillis(RadioStateMachine::kMasterHeartbeatInterval + 1);
  state_machine->RadioTick();
  packet = radio.getSentPacket();
  ASSERT_NE(packet, nullptr);
  EXPECT_EQ(packet->type, HEARTBEAT);
  delete packet;

  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

TEST_F(RadioStateMachineTest, doesntBecomeMasterIfReceivesPackets) {
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

TEST_F(RadioStateMachineTest, respectsClaimMaster) {
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

TEST_F(RadioStateMachineTest, doesElectionAndBecomesSlave) {
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

TEST_F(RadioStateMachineTest, doesElectionAndBecomesMaster) {
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
  delete received_packet;
}

TEST_F(RadioStateMachineTest, returnNetworkMillisForNoOffset) {
  setMillis(0);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 0);

  setMillis(12345);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 12345);
}

TEST_F(RadioStateMachineTest, slaveGetsTimeFromNetwork) {
  RadioPacket packet;
  packet.writeHeartbeat(10000);
  // The random ID for master election is in the range [2, 0xFFFF)
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetNetworkMillis(), 10000);

  setMillis(2000000);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 2010000);
}

TEST_F(RadioStateMachineTest, slaveGetsTimeFromNetwork_negativeOffset) {
  setMillis(10000);
  RadioPacket packet;
  packet.writeHeartbeat(0);
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetNetworkMillis(), 0);

  setMillis(2000000);
  EXPECT_EQ(state_machine->GetNetworkMillis(), 1990000);
}

TEST_F(RadioStateMachineTest, masterSendsSetEffect) {
  state_machine->SetNumEffects(255);
  state_machine->SetNumPalettes(255);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket *received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);
  delete received_packet;

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kChangeEffectInterval + 1);
  // First we'll get a heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);
  delete received_packet;

  // Then the set effect
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, SET_EFFECT);
  EXPECT_NE(received_packet->readEffectIndexFromSetEffect(), 0);
  EXPECT_EQ(received_packet->readDelayFromSetEffect(), 0);
  EXPECT_NE(received_packet->readPaletteIndexFromSetEffect(), 0);
  delete received_packet;

  setMillis(kMaxSlaveTimeout + RadioStateMachine::kChangeEffectInterval +
            RadioStateMachine::kBroadcastEffectInterval + 1);
  // Another heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);
  delete received_packet;

  // And now the same SetEffect
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, SET_EFFECT);
  EXPECT_NE(received_packet->readEffectIndexFromSetEffect(), 0);
  EXPECT_EQ(received_packet->readDelayFromSetEffect(), 0);
  EXPECT_NE(received_packet->readPaletteIndexFromSetEffect(), 0);
  delete received_packet;
}

TEST_F(RadioStateMachineTest, masterRespectsSetEffectDelay) {
  state_machine->SetNumEffects(255);

  setMillis(kMaxSlaveTimeout);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetCurrentState(), RadioState::Master);

  RadioPacket *received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);
  delete received_packet;

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
  delete received_packet;

  // Re-broadcasts the same set effect packet
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  received_packet->packet_id = setEffectPacket.packet_id;
  ASSERT_EQ(*received_packet, setEffectPacket);
  delete received_packet;

  setMillis(kMaxSlaveTimeout + 120 * 1000 + 1);
  // First we'll get a heartbeat
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, HEARTBEAT);
  delete received_packet;

  // Then the set effect
  state_machine->RadioTick();
  received_packet = radio.getSentPacket();
  ASSERT_NE(received_packet, nullptr);
  EXPECT_EQ(received_packet->type, SET_EFFECT);
  EXPECT_NE(received_packet->readEffectIndexFromSetEffect(), 0);
  delete received_packet;
}

TEST_F(RadioStateMachineTest, slaveReturnsEffectIndexFromNetwork) {
  EXPECT_EQ(state_machine->GetEffectIndex(), 0);

  RadioPacket packet;
  packet.writeSetEffect(42, 0, 0);
  radio.setReceivedPacket(&packet);
  state_machine->RadioTick();
  EXPECT_EQ(state_machine->GetEffectIndex(), 42);
}

TEST_F(RadioStateMachineTest, slaveReturnsSetEffectPacketFromNetwork) {
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

TEST_F(RadioStateMachineTest, SetEffect_Delay) {
  RadioPacket *defaultSetEffect = state_machine->GetSetEffect();

  RadioPacket setEffect;
  setEffect.writeSetEffect(1, /* delay= */ 5, 2);
  state_machine->SetEffect(&setEffect);
  state_machine->RadioTick();

  expectPacketsEqual(state_machine->GetSetEffect(), &setEffect);
  RadioPacket *sent_packet = radio.getSentPacket();
  expectPacketsEqual(sent_packet, &setEffect);
  delete sent_packet;

  setMillis(5 * 1000 + 1);
  state_machine->RadioTick();
  expectPacketsEqual(state_machine->GetSetEffect(), defaultSetEffect);
}

TEST_F(RadioStateMachineTest, SetEffect_NoDelay) {
  RadioPacket *defaultSetEffect = state_machine->GetSetEffect();

  RadioPacket setEffect;
  setEffect.writeSetEffect(1, /* delay= */ 0, 2);
  state_machine->SetEffect(&setEffect);
  state_machine->RadioTick();

  expectPacketsEqual(state_machine->GetSetEffect(), &setEffect);
  RadioPacket *sent_packet = radio.getSentPacket();
  expectPacketsEqual(sent_packet, &setEffect);
  delete sent_packet;

  setMillis(RadioStateMachine::kChangeEffectInterval + 1);
  state_machine->RadioTick();
  expectPacketsEqual(state_machine->GetSetEffect(), defaultSetEffect);
}
