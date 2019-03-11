#ifndef __RADIO_H__
#define __RADIO_H__

#include "Types.hpp"

enum PacketType {
  // Tells the slaves that they should blink the light.
  HEARTBEAT,

  // Sent by a master to claim mastership and tell all other masters to become
  // slaves. Usually sent as part of master negotiation.
  CLAIM_MASTER,

  // Used to set the pattern for the network. Contains which effect and
  // parameters for that effect. Typically sent by the master. Other devices
  // (e.g. the car remote) can also send this, and may include a flag for the
  // master to not change the pattern for a certain amount of time.
  SET_EFFECT,
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

  // Member functions to create and read specific packet types.

  // For HEARTBEAT
  void writeHeartbeat(uint32_t time);
  uint32_t readTimeFromHeartbeat();

  // For SET_EFFECT
  // delay: time for the master to not change the effect, in seconds
  void writeSetEffect(uint8_t effectIndex, uint8_t delay, uint8_t hue);
  uint8_t readEffectIndexFromSetEffect();
  uint8_t readDelayFromSetEffect();
  uint8_t readHueFromSetEffect();
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

  /**
   * Puts the radio into a low-power sleep mode. Wake up the radio by calling
   * readPacket or sendPacket.
   */
  virtual void sleep() = 0;
};

#endif
