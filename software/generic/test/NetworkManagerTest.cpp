#include "gtest/gtest.h"

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "FakeRadio.hpp"

TEST(NetworkManager, receive_noPackets) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioPacket packet;
  packet.packetId = 12345;
  packet.type = PING;
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
  radio.setReceivePacket(&receivedPacket);

  EXPECT_EQ(networkManager->receive(packet), true);
  EXPECT_EQ(packet, receivedPacket);
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
