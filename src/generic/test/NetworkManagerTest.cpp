#include <Radio.hpp>

#include "../NetworkManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

class NetworkManagerTest : public ::testing::Test {
 protected:
  void SetUp() override { networkManager = new NetworkManager(&radio); }

  void TearDown() override { delete networkManager; }

  void expectEqualAndDelete(RadioPacket *packet,
                            const RadioPacket &other_packet) {
    ASSERT_FALSE(packet == nullptr);
    EXPECT_EQ(*packet, other_packet);
    delete packet;
  }

  FakeRadio radio;
  NetworkManager *networkManager;
};

TEST_F(NetworkManagerTest, receive_noPackets) {
  RadioPacket packet;
  packet.packet_id = 12345;
  packet.type = HEARTBEAT;
  packet.dataLength = 1;
  packet.data[0] = 100;
  RadioPacket original_packet = packet;

  EXPECT_EQ(networkManager->receive(packet), false);
  EXPECT_EQ(packet, original_packet);
}

TEST_F(NetworkManagerTest, receive_setsPacket) {
  RadioPacket packet;

  RadioPacket received_packet;
  received_packet.packet_id = 12345;
  received_packet.dataLength = 1;
  radio.setReceivedPacket(&received_packet);

  EXPECT_EQ(networkManager->receive(packet), true);
  EXPECT_EQ(packet, received_packet);
}

TEST_F(NetworkManagerTest, receive_rebroadcasts) {
  RadioPacket packet;

  RadioPacket received_packet;
  received_packet.packet_id = 12345;
  received_packet.dataLength = 1;
  radio.setReceivedPacket(&received_packet);

  EXPECT_EQ(networkManager->receive(packet), true);
  RadioPacket *sent_packet = radio.getSentPacket();
  ASSERT_NE(sent_packet, nullptr);
  expectEqualAndDelete(sent_packet, received_packet);

  // With same ID, shouldn't rebroadcast again
  networkManager->receive(packet);
  EXPECT_EQ(radio.getSentPacket(), nullptr);

  // New id means it should rebroadcast
  received_packet.packet_id = 6789;
  radio.setReceivedPacket(&received_packet);
  EXPECT_EQ(networkManager->receive(packet), true);
  sent_packet = radio.getSentPacket();
  ASSERT_NE(sent_packet, nullptr);
  expectEqualAndDelete(sent_packet, received_packet);

  // Make sure it doesn't crash when exceeding the cache size
  // Start from 1 because 0 isn't a valid packet ID.
  for (uint16_t i = 1; i < NetworkManager::kRecentIdsCacheSize * 2; ++i) {
    received_packet.packet_id = i;
    radio.setReceivedPacket(&received_packet);
    EXPECT_EQ(networkManager->receive(packet), true);
    expectEqualAndDelete(radio.getSentPacket(), received_packet);

    EXPECT_EQ(networkManager->receive(packet), false);
    EXPECT_EQ(radio.getSentPacket(), nullptr);
  }
}

TEST_F(NetworkManagerTest, receive_doesntRebroadcastSentId) {
  RadioPacket sent_packet;
  sent_packet.dataLength = 1;
  networkManager->send(sent_packet);
  // Consume the FakeRadio's sent packet
  delete radio.getSentPacket();

  RadioPacket received_packet;
  received_packet.packet_id = sent_packet.packet_id;
  received_packet.dataLength = 1;
  radio.setReceivedPacket(&received_packet);

  RadioPacket packet;
  EXPECT_EQ(networkManager->receive(packet), false);
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

TEST_F(NetworkManagerTest, ReceivePausesRebroadcast) {
  RadioPacket received_packet;
  received_packet.packet_id = 1234;
  received_packet.dataLength = 1;
  radio.setReceivedPacket(&received_packet);

  RadioPacket packet;
  ASSERT_EQ(networkManager->receive(packet), true);
  RadioPacket *sent_packet = radio.getSentPacket();
  ASSERT_NE(sent_packet, nullptr);
  delete sent_packet;

  for (uint16_t n = 0; n < NetworkManager::kPacketCopiesThreshold + 1; n++) {
    radio.setReceivedPacket(&received_packet);
    ASSERT_EQ(networkManager->receive(packet), false);
  }

  received_packet.packet_id = 5678;
  radio.setReceivedPacket(&received_packet);
  ASSERT_EQ(networkManager->receive(packet), true);
  ASSERT_EQ(radio.getSentPacket(), nullptr);

  // Send more packets with few copies, to clear the buffer
  for (uint16_t n = 0; n < NetworkManager::kRecentIdsCacheSize; ++n) {
    received_packet.packet_id = 100 + n;
    radio.setReceivedPacket(&received_packet);
    ASSERT_EQ(networkManager->receive(packet), true);
    ASSERT_EQ(radio.getSentPacket(), nullptr);
  }

  advanceMillis(NetworkManager::kRebroadcastPauseMillis + 10);
  received_packet.packet_id = 1;
  radio.setReceivedPacket(&received_packet);
  ASSERT_EQ(networkManager->receive(packet), true);
  sent_packet = radio.getSentPacket();
  ASSERT_NE(sent_packet, nullptr);
  delete sent_packet;
}

TEST_F(NetworkManagerTest, send_sendsPacket) {
  RadioPacket packet;
  packet.packet_id = 0;
  packet.dataLength = 1;

  networkManager->send(packet);
  RadioPacket *sent_packet = radio.getSentPacket();
  EXPECT_NE(sent_packet->packet_id, 0);

  packet.packet_id = sent_packet->packet_id;
  EXPECT_EQ(*sent_packet, packet);
  delete sent_packet;
}
