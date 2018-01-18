#ifndef __RADIO_H__
#define __RADIO_H__

// Arduino provides the types of the form 'int8_t'. In vanilla C++, we need to
// include these manually.
#ifndef ARDUINO
#include <cstdint>
#endif

enum PacketType {
  PING,
};

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
  uint8_t data[58];
};

/** TODO: create an interface for the radio. */
class Radio {
 public:
  /**
   * If a packet is avilable, reads it into the provided struct and return
   * true. If no packet is available, returns false.
   */
  virtual bool readPacket(RadioPacket &packet) = 0;

  /**
   * Sends the packet.
   */
  virtual void sendPacket(RadioPacket &packet) = 0;
};

#endif
