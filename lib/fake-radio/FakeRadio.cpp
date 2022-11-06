#include "FakeRadio.hpp"

#include <cstdio>

FakeRadio::FakeRadio() {
  received_packet = nullptr;
  sent_packet = nullptr;
}

FakeRadio::~FakeRadio() {
  if (sent_packet != nullptr) {
    delete sent_packet;
  }
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
  if (sent_packet == nullptr) {
    sent_packet = new RadioPacket();
  }
  *sent_packet = packet;
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
