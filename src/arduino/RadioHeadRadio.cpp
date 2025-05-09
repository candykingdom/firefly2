#include "RadioHeadRadio.hpp"

#include <RH_RF69.h>

#include <Debug.hpp>

bool RadioHeadRadio::Begin() {
  if (!radio.init()) {
    debug_printf("Failed to initialize radio");
    return false;
  }
  radio.setTxPower(13, false);
  radio.setFrequency(915.0);
  radio.available();
  return true;
}

bool RadioHeadRadio::readPacket(RadioPacket &packet) {
  static std::array<uint8_t, kMaxFifoSizePacketSize> buffer;
  if (!radio.available()) {
    return false;
  }

  uint8_t received_length = 0;
  received_length = kMaxFifoSizePacketSize;
  if (radio.recv(buffer.data(), &received_length)) {
    if (received_length > kFrontPacketPadding) {
      packet.packet_id = (buffer[0] << 8) + buffer[1];
      packet.type = (PacketType)buffer[2];

      // Remember back in school when they told you that volatile was a word you
      // should stray from because it will make everything incompatible? Yep --
      // memcopy is incompatible. If you wanna copy data off of the volatile
      // section then you have to hand roll it yourself.
      for (byte i = kFrontPacketPadding; i < received_length; i++) {
        packet.data[i - kFrontPacketPadding] = buffer[i];
      }
      radio.available();
      return true;
    }
  }
  radio.available();
  return false;
}

void RadioHeadRadio::sendPacket(RadioPacket &packet) {
  static std::array<uint8_t, kMaxPacketSize> buffer;

  buffer[0] = packet.packet_id >> 8;      // Take the top 8 bits.
  buffer[1] = packet.packet_id & 0x00ff;  // Mask off the top 8 bits.
  buffer[2] = packet.type;

  // Now that we have consumed the first 3 bytes of data, memcpy past the
  // consumed part and write into the rest of the buffer.
  if (packet.dataLength > 0) {
    memcpy(buffer.data() + kFrontPacketPadding, packet.data.data(),
           packet.dataLength);
  }

  radio.send(buffer.data(), packet.dataLength + kFrontPacketPadding);

  // Go back into RX mode
  radio.available();
}

void RadioHeadRadio::sleep() { radio.sleep(); }

int16_t RadioHeadRadio::LastRssi() { return radio.lastRssi(); }
