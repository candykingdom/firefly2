#ifndef __RADIO_H__
#define __RADIO_H__

#include "Types.hpp"

enum PacketType {
  // Tells the slaves that they should blink the light.
  HEARTBEAT,
};

static const uint8_t PACKET_DATA_LENGTH = 58;

struct RadioPacket {
  /**
   * This is used for "mesh" networking. Upon receiving a packet, each node
   * rebroadcasts it if it has not recently rebroadcasted a packet with that
   * packetId.
   */
  uint16_t packetId;

  /**
   * The type of packet. This determines how the data is processed.
   */
  PacketType type;

  /**
   * The length of the data field.
   */
  uint8_t dataLength;

  /**
   * The raw application-layer data. The length is the max packet length (61)
   * minus the size of the packet ID and type (3 bytes).
   */
  uint8_t data[PACKET_DATA_LENGTH];
};

inline bool operator==(const RadioPacket& lhs, const RadioPacket& rhs) {
  // Data length too big -> invalid packet, so return false
  if (lhs.dataLength > PACKET_DATA_LENGTH ||
      rhs.dataLength > PACKET_DATA_LENGTH) {
    return false;
  }

  return lhs.packetId == rhs.packetId && lhs.type == rhs.type &&
         lhs.dataLength == rhs.dataLength &&
         !memcmp(lhs.data, rhs.data, lhs.dataLength);
}

/** TODO: create an interface for the radio. */
class Radio {
 public:
  /**
   * If a packet is avilable, reads it into the provided struct and return
   * true. If no packet is available, returns false.
   */
  virtual bool readPacket(RadioPacket& packet) = 0;

  /**
   * Sends the packet.
   */
  virtual void sendPacket(RadioPacket& packet) = 0;
};

#endif
