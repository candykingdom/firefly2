#include "NetworkManager.hpp"

#include <Debug.hpp>

NetworkManager::NetworkManager(Radio *const radio) : radio(radio) {
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    recentIdsCache[i] = 0;
  }
  recent_ids_cache_index = 0;
}

bool NetworkManager::receive(RadioPacket &packet) {
  if (!radio->readPacket(packet)) {
    return false;
  }

  // Rebroadcast the packet if we haven't seen it's ID recently.
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    if (recentIdsCache[i] == packet.packet_id) {
      return false;
    }
  }

  radio->sendPacket(packet);
  AddToRecentIdsCache(packet.packet_id);

  return true;
}

void NetworkManager::send(RadioPacket &packet) {
  // [2, 0xFFFF) allow us to use packet ID 1 in tests, so that the code under
  // test always wins master election.
  packet.packet_id = random(2, 0xFFFF);
  radio->sendPacket(packet);
  AddToRecentIdsCache(packet.packet_id);
}

void NetworkManager::AddToRecentIdsCache(uint16_t id) {
  recentIdsCache[recent_ids_cache_index++] = id;
  recent_ids_cache_index %= kRecentIdsCacheSize;
}
