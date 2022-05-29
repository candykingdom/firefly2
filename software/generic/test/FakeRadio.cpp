#include "FakeRadio.hpp"

FakeRadio::FakeRadio() {
  received_packet = nullptr;
  sent_packet = nullptr;
}

bool FakeRadio::readPacket(RadioPacket &packet) {
  if (received_packet == nullptr) {
    return false;
  } else {
    memcpy(&packet, received_packet, sizeof(RadioPacket));
    return true;
  }
}

void FakeRadio::sendPacket(RadioPacket &packet) {
  // Note: this is sort-of wrong, since it just copies the pointer, not the
  // struct itself. However, since this is only used in tests, it's OK.
  sent_packet = new RadioPacket();
  memcpy(sent_packet, &packet, sizeof(RadioPacket));
}

void FakeRadio::setReceivedPacket(RadioPacket *packet) {
  received_packet = packet;
  if (packet == nullptr) {
    return;
  }

  // Manually clear the rest of the data buffer, in case code under test is
  // mis-sizing packets.
  for (int i = received_packet->dataLength; i < PACKET_DATA_LENGTH; i++) {
    received_packet->data[i] = 0;
  }
}

RadioPacket *FakeRadio::getSentPacket() {
  RadioPacket *thePacket = sent_packet;
  sent_packet = nullptr;
  return thePacket;
}
