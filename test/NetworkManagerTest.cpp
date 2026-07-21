#include <Radio.hpp>

#include "FakeRadio.hpp"
#include "FireflyNetworkManager.hpp"
#include "gtest/gtest.h"

class FireflyNetworkManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  void expectEqualAndDelete(RadioPacket *packet,
                            const RadioPacket &other_packet) {
    EXPECT_EQ(*packet, other_packet);
    delete packet;
  }

  FakeRadio radio;
  FireflyNetworkManager networkManager{&radio};
};

TEST_F(FireflyNetworkManagerTest, receive_noPackets) {
  RadioPacket packet;
  packet.packet_id = 12345;
  packet.type = HEARTBEAT;
  packet.dataLength = 1;
  packet.data[0] = 100;
  RadioPacket original_packet = packet;

  EXPECT_EQ(networkManager.receive(packet), false);
  EXPECT_EQ(packet, original_packet);
}

TEST_F(FireflyNetworkManagerTest, receive_setsPacket) {
  RadioPacket packet;

  RadioPacket received_packet;
  received_packet.packet_id = 12345;
  received_packet.writeHeartbeat(1);
  radio.setReceivedPacket(&received_packet);

  EXPECT_EQ(networkManager.receive(packet), true);
  EXPECT_EQ(packet, received_packet);
}

TEST_F(FireflyNetworkManagerTest, receive_rebroadcasts) {
  RadioPacket packet;

  RadioPacket received_packet;
  received_packet.packet_id = 12345;
  received_packet.writeHeartbeat(1);
  radio.setReceivedPacket(&received_packet);

  EXPECT_EQ(networkManager.receive(packet), true);
  expectEqualAndDelete(radio.getSentPacket(), received_packet);

  // With same ID, shouldn't rebroadcast again
  networkManager.receive(packet);
  EXPECT_EQ(radio.getSentPacket(), nullptr);

  // New id means it should rebroadcast
  received_packet.packet_id = 6789;
  radio.setReceivedPacket(&received_packet);
  EXPECT_EQ(networkManager.receive(packet), true);
  expectEqualAndDelete(radio.getSentPacket(), received_packet);

  // Make sure it doesn't crash when exceeding the cache size
  // Start from 1 because 0 isn't a valid packet ID.
  for (uint16_t i = 1; i < RadioWrapper::kRecentIdsCacheSize * 2; i++) {
    received_packet.packet_id = i;
    radio.setReceivedPacket(&received_packet);
    EXPECT_EQ(networkManager.receive(packet), true);
    expectEqualAndDelete(radio.getSentPacket(), received_packet);

    EXPECT_EQ(networkManager.receive(packet), false);
    EXPECT_EQ(radio.getSentPacket(), nullptr);
  }
}

TEST_F(FireflyNetworkManagerTest, receive_doesntRebroadcastSentId) {
  RadioPacket sent_packet;
  sent_packet.packet_id = 1;
  sent_packet.writeHeartbeat(1);
  networkManager.send(sent_packet);
  // Consume the FakeRadio's sent packet
  delete radio.getSentPacket();

  RadioPacket received_packet;
  received_packet.packet_id = sent_packet.packet_id;
  received_packet.writeHeartbeat(1);
  radio.setReceivedPacket(&received_packet);

  RadioPacket packet;
  EXPECT_EQ(networkManager.receive(packet), false);
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

// A point-to-point bridge radio (RebroadcastsToSource() == false) must not echo
// a packet back out the port it arrived on, but must still forward it onto the
// other radio so traffic crosses the bridge.
TEST_F(FireflyNetworkManagerTest, receive_pointToPointSourceForwardsButNoEcho) {
  FakeRadio broadcast_radio;
  FakeRadio serial_radio;
  serial_radio.SetRebroadcastsToSource(false);

  FireflyNetworkManager nm{&broadcast_radio};
  nm.addRadio(&serial_radio);

  // broadcast_radio (radios[0]) has nothing, so receive() reads from the
  // point-to-point serial_radio (radios[1]) -- making it the source.
  RadioPacket received_packet;
  received_packet.packet_id = 4242;
  received_packet.writeHeartbeat(1);
  serial_radio.setReceivedPacket(&received_packet);

  RadioPacket packet;
  EXPECT_EQ(nm.receive(packet), true);

  // Forwarded onto the broadcast radio (crosses the bridge)...
  expectEqualAndDelete(broadcast_radio.getSentPacket(), received_packet);
  // ...but not echoed back out the serial radio it arrived on.
  EXPECT_EQ(serial_radio.getSentPacket(), nullptr);
}

// The converse: a packet arriving on a shared broadcast radio must be re-sent
// on that radio (multi-hop flood) *and* forwarded onto the point-to-point
// bridge radio, whose RebroadcastsToSource() only suppresses same-source echo.
TEST_F(FireflyNetworkManagerTest,
       receive_broadcastSourceFloodsAndCrossesBridge) {
  FakeRadio broadcast_radio;
  FakeRadio serial_radio;
  serial_radio.SetRebroadcastsToSource(false);

  FireflyNetworkManager nm{&broadcast_radio};
  nm.addRadio(&serial_radio);

  // Packet arrives on broadcast_radio (radios[0], the source).
  RadioPacket received_packet;
  received_packet.packet_id = 777;
  received_packet.writeHeartbeat(1);
  broadcast_radio.setReceivedPacket(&received_packet);

  RadioPacket packet;
  EXPECT_EQ(nm.receive(packet), true);

  // Re-sent on the broadcast radio (flood keeps multi-hop mesh working)...
  expectEqualAndDelete(broadcast_radio.getSentPacket(), received_packet);
  // ...and forwarded onto the serial bridge radio, even though its own
  // RebroadcastsToSource() is false -- it isn't the source here.
  expectEqualAndDelete(serial_radio.getSentPacket(), received_packet);
}

// receive() reads at most one radio per call. If it always scanned from
// radios[0], a consistently busy radio would starve the others behind it -- a
// packet on radios[1] would never be delivered while radios[0] had traffic.
// The round-robin cursor must give every radio a turn.
TEST_F(FireflyNetworkManagerTest, receive_roundRobinsAcrossBusyRadios) {
  FakeRadio radio_a;
  FakeRadio radio_b;

  FireflyNetworkManager nm{&radio_a};
  nm.addRadio(&radio_b);

  // Both radios are permanently busy with distinct packet ids.
  RadioPacket packet_a;
  packet_a.packet_id = 100;
  packet_a.writeHeartbeat(1);
  radio_a.setReceivedPacket(&packet_a);

  RadioPacket packet_b;
  packet_b.packet_id = 200;
  packet_b.writeHeartbeat(2);
  radio_b.setReceivedPacket(&packet_b);

  // Two calls must surface a packet from each radio, not the same one twice.
  RadioPacket packet;
  ASSERT_EQ(nm.receive(packet), true);
  const uint16_t first_id = packet.packet_id;
  ASSERT_EQ(nm.receive(packet), true);
  const uint16_t second_id = packet.packet_id;

  EXPECT_NE(first_id, second_id);
  EXPECT_TRUE((first_id == 100 && second_id == 200) ||
              (first_id == 200 && second_id == 100));
}

TEST_F(FireflyNetworkManagerTest, send_sendsPacket) {
  RadioPacket packet;
  packet.packet_id = 0;
  packet.writeHeartbeat(1);

  networkManager.send(packet);
  RadioPacket *sent_packet = radio.getSentPacket();
  EXPECT_NE(sent_packet->packet_id, 0);

  packet.packet_id = sent_packet->packet_id;
  EXPECT_EQ(*sent_packet, packet);
  delete sent_packet;
}

TEST_F(FireflyNetworkManagerTest, receive_dropsMalformedPackets) {
  // A truncated HEARTBEAT: enough bytes to frame and decode, not enough to
  // carry a timestamp. Must not reach the caller or be flooded onward.
  RadioPacket truncated;
  truncated.packet_id = 12345;
  truncated.writeHeartbeat(1);
  truncated.dataLength = HEARTBEAT_DATA_LENGTH - 1;
  radio.setReceivedPacket(&truncated);

  RadioPacket packet;
  EXPECT_EQ(networkManager.receive(packet), false);
  EXPECT_EQ(radio.getSentPacket(), nullptr);
}

// A node must keep relaying packet types it doesn't understand, so that
// firmware older than a newly-added type doesn't become a black hole for it in
// a mixed-firmware mesh. RadioStateMachine ignores types it can't dispatch.
TEST_F(FireflyNetworkManagerTest, receive_forwardsUnknownPacketTypes) {
  RadioPacket unknown;
  unknown.packet_id = 12345;
  unknown.writeHeartbeat(1);
  unknown.type = (PacketType)0x7E;
  radio.setReceivedPacket(&unknown);

  RadioPacket packet;
  EXPECT_EQ(networkManager.receive(packet), true);
  expectEqualAndDelete(radio.getSentPacket(), unknown);
}
