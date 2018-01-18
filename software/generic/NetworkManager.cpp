#include "NetworkManager.hpp"

NetworkManager::NetworkManager(Radio *const radio) : radio(radio) {}

bool NetworkManager::receive(RadioPacket &packet) {
  if (!radio->readPacket(packet)) {
    return false;
  }

  return true;
}

void NetworkManager::send(RadioPacket &packet) {
  packet.packetId = random(1, 0xFFFF);
  radio->sendPacket(packet);
}
