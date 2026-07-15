#include "gtest/gtest.h"

#define DEBUG 1
#include <Radio.hpp>

TEST(RadioPacket, serializesHeartbeat0) {
  RadioPacket packet;

  packet.writeHeartbeat(0);
  EXPECT_EQ(0, packet.readTimeFromHeartbeat());
}

TEST(RadioPacket, serializesHeartbeat1) {
  RadioPacket packet;

  packet.writeHeartbeat(1);
  EXPECT_EQ(1, packet.readTimeFromHeartbeat());
}

TEST(RadioPacket, serializesHeartbeatBig) {
  RadioPacket packet;

  const uint32_t test = 0x81234567;
  packet.writeHeartbeat(test);
  EXPECT_EQ(test, packet.readTimeFromHeartbeat());
}

TEST(RadioPacket, serializesHeartbeatBoundaryTimes) {
  // Times with the top bit set exercise the decode's high-byte shift, which
  // must be well-defined for all 2^32 values in every language mode the
  // firmware builds under (the SAMD node target uses gnu++11, where an
  // uncast `byte << 24` shifts into the sign bit of int).
  const uint32_t times[] = {0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF};

  for (uint32_t time : times) {
    RadioPacket packet;
    packet.writeHeartbeat(time);
    EXPECT_EQ(time, packet.readTimeFromHeartbeat());
  }
}

// Wire-codec conformance tests. See
// specs/002-fix-audit-findings/contracts/wire-format.md.

namespace {

// Builds a packet with an arbitrary payload of the given length.
RadioPacket MakePacket(uint16_t id, PacketType type, uint8_t data_length) {
  RadioPacket packet;
  packet.packet_id = id;
  packet.type = type;
  packet.dataLength = data_length;
  for (uint8_t i = 0; i < data_length; i++) {
    packet.data[i] = i * 7 + 1;
  }
  return packet;
}

}  // namespace

TEST(RadioPacket, serializeDeserializeRoundTripsAllTypesAndLengths) {
  const PacketType types[] = {HEARTBEAT, CLAIM_MASTER, SET_EFFECT, SET_CONTROL};
  const uint8_t lengths[] = {0, 1, 4, PACKET_DATA_LENGTH};

  for (PacketType type : types) {
    for (uint8_t length : lengths) {
      RadioPacket original = MakePacket(0xBEEF, type, length);

      std::array<uint8_t, PACKET_DATA_LENGTH + 3> wire;
      const uint8_t wire_length = original.Serialize(wire.data());
      EXPECT_EQ(length + 3, wire_length);

      RadioPacket decoded;
      ASSERT_TRUE(decoded.Deserialize(wire.data(), wire_length));
      EXPECT_EQ(original.packet_id, decoded.packet_id);
      EXPECT_EQ(original.type, decoded.type);
      EXPECT_EQ(original.dataLength, decoded.dataLength);
      EXPECT_TRUE(original == decoded);
    }
  }
}

TEST(RadioPacket, deserializeThenReserializeIsByteIdentical) {
  // Hand-built wire frame: id 0xABCD, type SET_EFFECT, 3-byte payload.
  const uint8_t wire[] = {0xAB, 0xCD, SET_EFFECT, 5, 60, 12};

  RadioPacket packet;
  ASSERT_TRUE(packet.Deserialize(wire, sizeof(wire)));
  EXPECT_EQ(0xABCD, packet.packet_id);
  EXPECT_EQ(SET_EFFECT, packet.type);
  EXPECT_EQ(3, packet.dataLength);

  // Re-encoding (what a mesh rebroadcast transmits) must reproduce the
  // original frame exactly.
  std::array<uint8_t, PACKET_DATA_LENGTH + 3> reencoded;
  const uint8_t reencoded_length = packet.Serialize(reencoded.data());
  ASSERT_EQ(sizeof(wire), reencoded_length);
  EXPECT_EQ(0, memcmp(wire, reencoded.data(), sizeof(wire)));
}

TEST(RadioPacket, deserializeRejectsTruncatedAndOversizedFrames) {
  std::array<uint8_t, 255> wire;
  wire.fill(0x42);

  RadioPacket packet;
  EXPECT_FALSE(packet.Deserialize(wire.data(), 0));
  EXPECT_FALSE(packet.Deserialize(wire.data(), 1));
  EXPECT_FALSE(packet.Deserialize(wire.data(), 2));
  // 3-byte header + 58-byte payload = 61 is the maximum valid frame.
  EXPECT_FALSE(packet.Deserialize(wire.data(), PACKET_DATA_LENGTH + 4));
  EXPECT_FALSE(packet.Deserialize(wire.data(), 255));
}

TEST(RadioPacket, deserializeAcceptsHeaderOnlyFrame) {
  const uint8_t wire[] = {0x00, 0x07, CLAIM_MASTER};

  RadioPacket packet;
  ASSERT_TRUE(packet.Deserialize(wire, sizeof(wire)));
  EXPECT_EQ(7, packet.packet_id);
  EXPECT_EQ(CLAIM_MASTER, packet.type);
  EXPECT_EQ(0, packet.dataLength);

  std::array<uint8_t, PACKET_DATA_LENGTH + 3> reencoded;
  ASSERT_EQ(3, packet.Serialize(reencoded.data()));
  EXPECT_EQ(0, memcmp(wire, reencoded.data(), sizeof(wire)));
}

TEST(RadioPacket, deserializeToleratesUnknownTypeByte) {
  // Invalid/unknown packets must never crash (CLAUDE.md invariant); the
  // codec stores unknown type bytes as-is for higher layers to ignore.
  const uint8_t wire[] = {0x12, 0x34, 0xEE, 1, 2, 3};

  RadioPacket packet;
  ASSERT_TRUE(packet.Deserialize(wire, sizeof(wire)));
  EXPECT_EQ(3, packet.dataLength);
}

TEST(RadioPacket, serializeClampsCorruptOversizedDataLength) {
  // Serialize's contract: clamp rather than read past the payload array if
  // handed a corrupt packet.
  RadioPacket packet = MakePacket(0x0002, HEARTBEAT, PACKET_DATA_LENGTH);
  packet.dataLength = 200;

  std::array<uint8_t, PACKET_DATA_LENGTH + 3> wire;
  EXPECT_EQ(PACKET_DATA_LENGTH + 3, packet.Serialize(wire.data()));
}

TEST(RadioPacket, relayedHeartbeatDecodesToSameNetworkTime) {
  // A master's heartbeat relayed through an intermediate node must give a
  // third node the same network time as hearing the master directly.
  const uint32_t network_time = 0x12345678;
  RadioPacket master_packet;
  master_packet.packet_id = 100;
  master_packet.writeHeartbeat(network_time);

  std::array<uint8_t, PACKET_DATA_LENGTH + 3> master_wire;
  const uint8_t master_wire_length =
      master_packet.Serialize(master_wire.data());

  // Intermediate node receives it...
  RadioPacket relay_packet;
  ASSERT_TRUE(relay_packet.Deserialize(master_wire.data(), master_wire_length));

  // ...and rebroadcasts it (NetworkManager re-sends the received packet).
  std::array<uint8_t, PACKET_DATA_LENGTH + 3> relay_wire;
  const uint8_t relay_wire_length = relay_packet.Serialize(relay_wire.data());
  ASSERT_EQ(master_wire_length, relay_wire_length);

  // A far node that only hears the relay decodes the same time.
  RadioPacket far_packet;
  ASSERT_TRUE(far_packet.Deserialize(relay_wire.data(), relay_wire_length));
  EXPECT_EQ(network_time, far_packet.readTimeFromHeartbeat());
}

TEST(RadioPacket, serializesSetEffect0) {
  RadioPacket packet;

  packet.writeSetEffect(0, 1, 0);
  EXPECT_EQ(0, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(1, packet.readDelayFromSetEffect());
  EXPECT_EQ(0, packet.readPaletteIndexFromSetEffect());

  packet.writeSetEffect(1, 0, 0);
  EXPECT_EQ(1, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(0, packet.readDelayFromSetEffect());
  EXPECT_EQ(0, packet.readPaletteIndexFromSetEffect());

  packet.writeSetEffect(0, 0, 1);
  EXPECT_EQ(0, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(0, packet.readDelayFromSetEffect());
  EXPECT_EQ(1, packet.readPaletteIndexFromSetEffect());

  packet.writeSetEffect(250, 199, 54);
  EXPECT_EQ(250, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(199, packet.readDelayFromSetEffect());
  EXPECT_EQ(54, packet.readPaletteIndexFromSetEffect());
}
