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
    // Note: header-only frames (received_length == kFrontPacketPadding) are
    // dropped here, as they always have been.
    if (received_length > kFrontPacketPadding &&
        packet.Deserialize(buffer.data(), received_length)) {
      radio.available();
      return true;
    }
  }
  radio.available();
  return false;
}

void RadioHeadRadio::sendPacket(RadioPacket &packet) {
  static std::array<uint8_t, kMaxPacketSize> buffer;

  const uint8_t wire_length = packet.Serialize(buffer.data());
  radio.send(buffer.data(), wire_length);

  // Go back into RX mode
  radio.available();
}

void RadioHeadRadio::sleep() { radio.sleep(); }

int16_t RadioHeadRadio::LastRssi() { return radio.lastRssi(); }
