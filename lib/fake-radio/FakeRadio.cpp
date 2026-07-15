#include "FakeRadio.hpp"

#include <cstdio>

FakeRadio::FakeRadio() {
  received_packet = nullptr;
  sent_wire_length = 0;
  has_sent_packet = false;
}

FakeRadio::~FakeRadio() {}

bool FakeRadio::readPacket(RadioPacket &packet) {
  if (received_packet == nullptr) {
    return false;
  } else {
    memcpy(&packet, received_packet, sizeof(RadioPacket));
    return true;
  }
}

void FakeRadio::sendPacket(RadioPacket &packet) {
  sent_wire_length = packet.Serialize(sent_wire);
  has_sent_packet = true;
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
  if (!has_sent_packet) {
    return nullptr;
  }
  has_sent_packet = false;

  RadioPacket *packet = new RadioPacket();
  if (!packet->Deserialize(sent_wire, sent_wire_length)) {
    delete packet;
    return nullptr;
  }
  return packet;
}
