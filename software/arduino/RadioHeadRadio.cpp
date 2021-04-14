#include "RadioHeadRadio.hpp"

#include <RH_RF69.h>

#include "../generic/Debug.hpp"

RadioHeadRadio::RadioHeadRadio() {
  radio.init();
  radio.setTxPower(13, false);
  radio.setFrequency(915.0);
  radio.available();
}

bool RadioHeadRadio::readPacket(RadioPacket &packet) {
  static uint8_t buffer[kMaxFifoSizePacketSize];
  if (!radio.available()) {
    return false;
  }

  uint8_t receivedLength = 0;
  receivedLength = kMaxFifoSizePacketSize;
  if (radio.recv(buffer, &receivedLength)) {
    if (receivedLength > kFrontPacketPadding) {
      packet.packetId = (buffer[0] << 8) + buffer[1];
      packet.type = (PacketType)buffer[2];

      // Remember back in school when they told you that volatile was a word you
      // should stray from because it will make everything incompatible? Yep --
      // memcopy is incompatible. If you wanna copy data off of the volatile
      // section then you have to hand roll it yourself.
      for (byte i = kFrontPacketPadding; i < receivedLength; i++) {
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
  static uint8_t buffer[kMaxPacketSize];

  buffer[0] = packet.packetId >> 8;      // Take the top 8 bits.
  buffer[1] = packet.packetId & 0x00ff;  // Mask off the top 8 bits.
  buffer[2] = packet.type;

  // Now that we have consumed the first 3 bytes of data, memcpy past the
  // consumed part and write into the rest of the buffer.
  if (packet.dataLength > 0) {
    memcpy(buffer + kFrontPacketPadding, packet.data, packet.dataLength);
  }

  radio.send((uint8_t *)buffer, packet.dataLength + kFrontPacketPadding);

  // Go back into RX mode
  radio.available();
}

void RadioHeadRadio::sleep() { radio.sleep(); }

int16_t RadioHeadRadio::LastRssi() { return radio.lastRssi(); }
