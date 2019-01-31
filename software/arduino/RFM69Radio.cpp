#include "RFM69Radio.hpp"
#include <RFM69.h>
#include "Debug.hpp"

RFM69Radio::RFM69Radio() {
  radio = new RFM69(SS, /* DIO */ 5, /* is HW */ false, /* SPIClass */ &SPI);
  if (!radio->initialize(RF69_868MHZ, kNodeId, kNetworkId)) {
    // TODO(achew): Actually do this.
    debug_printf("Unable to initialize RFM69. Make this be better");
  }
  // TODO: set this higher when hardware issues are resolved. When this is set
  // above 5dBm, voltage regulator is unstable?
  radio->setPowerLevel(20);

  radio->promiscuous(true);
}

bool RFM69Radio::readPacket(RadioPacket &packet) {
  if (radio->receiveDone() && radio->DATALEN > 0) {
    packet.packetId = (radio->DATA[0] << 8) + radio->DATA[1];
    packet.type = (PacketType)radio->DATA[2];

    // Remember back in school when they told you that volatile was a word you
    // should stray from because it will make everything incompatible? Yep --
    // memcopy is incompatible. If you wanna copy data off of the volatile
    // section then you have to hand roll it yourself.
    for (byte i = kFrontPacketPadding; i < radio->DATALEN; i++) {
      packet.data[i - kFrontPacketPadding] = radio->DATA[i];
    }
    return true;
  }
  return false;
}

void RFM69Radio::sendPacket(RadioPacket &packet) {
  static uint8_t buffer[kMaxPacketSize];

  buffer[0] = packet.packetId >> 8;      // Take the top 8 bits.
  buffer[1] = packet.packetId & 0x00ff;  // Mask off the top 8 bits.
  buffer[2] = packet.type;

  // Now that we have consumed the first 3 bytes of data, memcpy past the
  // consumed part and write into the rest of the buffer.
  if (packet.dataLength > 0) {
    memcpy(buffer + kFrontPacketPadding, packet.data, packet.dataLength);
  }

  radio->send(kNodeId, buffer, kFrontPacketPadding + packet.dataLength);
}
