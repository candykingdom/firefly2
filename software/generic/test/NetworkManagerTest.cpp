#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "FakeRadio.hpp"

TEST(NetworkManager, receive_noPackets) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioPacket packet;
  packet.packetId = 12345;
  packet.type = HEARTBEAT;
  packet.dataLength = 1;
  packet.data[0] = 100;
  RadioPacket originalPacket = packet;

  EXPECT_EQ(networkManager->receive(packet), false);
  EXPECT_EQ(packet, originalPacket);
}

TEST(NetworkManager, receive_setsPacket) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioPacket packet;

  RadioPacket receivedPacket;
  receivedPacket.packetId = 12345;
  receivedPacket.dataLength = 1;
  radio.setReceivedPacket(&receivedPacket);

  EXPECT_EQ(networkManager->receive(packet), true);
  EXPECT_EQ(packet, receivedPacket);
}

TEST(NetworkManager, receive_rebroadasts) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioPacket packet;

  RadioPacket receivedPacket;
  receivedPacket.packetId = 12345;
  receivedPacket.dataLength = 1;
  radio.setReceivedPacket(&receivedPacket);

  EXPECT_EQ(networkManager->receive(packet), true);
  EXPECT_EQ(*radio.getSentPacket(), receivedPacket);

  // With same ID, shouldn't rebroadcast again
  networkManager->receive(packet);
  EXPECT_EQ(radio.getSentPacket(), nullptr);

  // New id means it should rebroadcast
  receivedPacket.packetId = 6789;
  radio.setReceivedPacket(&receivedPacket);
  EXPECT_EQ(networkManager->receive(packet), true);
  EXPECT_EQ(*radio.getSentPacket(), receivedPacket);

  // Make sure it doesn't crash when exceeding the cache size
  // Start from 1 because 0 isn't a valid packet ID.
  for (uint16_t i = 1; i < NetworkManager::kRecentIdsCacheSize * 2; i++) {
    receivedPacket.packetId = i;
    radio.setReceivedPacket(&receivedPacket);
    EXPECT_EQ(networkManager->receive(packet), true);
    EXPECT_EQ(*radio.getSentPacket(), receivedPacket);

    EXPECT_EQ(networkManager->receive(packet), true);
    EXPECT_EQ(radio.getSentPacket(), nullptr);
  }
}

TEST(NetworkManager, receive_doesntRebroadcastSentId) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioPacket sentPacket;
  sentPacket.dataLength = 1;
  networkManager->send(sentPacket);
  // Consume the FakeRadio's sent packet
  radio.getSentPacket();

  RadioPacket receivedPacket;
  receivedPacket.packetId = sentPacket.packetId;
  receivedPacket.dataLength = 1;
  radio.setReceivedPacket(&receivedPacket);

  RadioPacket packet;
  EXPECT_EQ(networkManager->receive(packet), true);
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

TEST(NetworkManager, send_sendsPacket) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioPacket packet;
  packet.packetId = 0;
  packet.dataLength = 1;

  networkManager->send(packet);
  RadioPacket *sentPacket = radio.getSentPacket();
  EXPECT_NE(sentPacket->packetId, 0);

  packet.packetId = sentPacket->packetId;
  EXPECT_EQ(*sentPacket, packet);
}
