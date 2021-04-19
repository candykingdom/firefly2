#include <Radio.hpp>

FakeRadio::FakeRadio() {
  receivedPacket = nullptr;
  sentPacket = nullptr;
}

bool FakeRadio::readPacket(RadioPacket &packet) {
  if (receivedPacket == nullptr) {
    return false;
  } else {
    memcpy(&packet, receivedPacket, sizeof(RadioPacket));
    return true;
  }
}

void FakeRadio::sendPacket(RadioPacket &packet) {
  // Note: this is sort-of wrong, since it just copies the pointer, not the
  // struct itself. However, since this is only used in tests, it's OK.
  sentPacket = new RadioPacket();
  memcpy(sentPacket, &packet, sizeof(RadioPacket));
}

void FakeRadio::setReceivedPacket(RadioPacket *packet) {
  receivedPacket = packet;
  if (packet == nullptr) {
    return;
  }

  // Manually clear the rest of the data buffer, in case code under test is
  // mis-sizing packets.
  for (int i = receivedPacket->dataLength; i < PACKET_DATA_LENGTH; i++) {
    receivedPacket->data[i] = 0;
  }
}

RadioPacket *FakeRadio::getSentPacket() {
  RadioPacket *thePacket = sentPacket;
  sentPacket = nullptr;
  return thePacket;
}
