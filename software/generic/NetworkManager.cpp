#include "NetworkManager.hpp"

NetworkManager::NetworkManager(Radio *const radio) : radio(radio) {
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    recentIdsCache[i] = 0;
  }
  recentIdsCacheIndex = 0;
}

bool NetworkManager::receive(RadioPacket &packet) {
  if (!radio->readPacket(packet)) {
    return false;
  }

  // Rebroadcast the packet if we haven't seen it's ID recently.
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    if (recentIdsCache[i] == packet.packetId) {
      return false;
    }
  }

  radio->sendPacket(packet);
  AddToRecentIdsCache(packet.packetId);

  return true;
}

void NetworkManager::send(RadioPacket &packet) {
  // [2, 0xFFFF) allow us to use packet ID 1 in tests, so that the code under
  // test always wins master election.
  packet.packetId = random(2, 0xFFFF);
  radio->sendPacket(packet);
  AddToRecentIdsCache(packet.packetId);
}

void NetworkManager::AddToRecentIdsCache(uint16_t id) {
  recentIdsCache[recentIdsCacheIndex++] = id;
  recentIdsCacheIndex %= kRecentIdsCacheSize;
}
