#include "FakeRadio.hpp"

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

void FakeRadio::setReceivePacket(RadioPacket *packet) {
  receivedPacket = packet;
}

RadioPacket *FakeRadio::getSentPacket() {
  RadioPacket *thePacket = sentPacket;
  sentPacket = nullptr;
  return thePacket;
}
