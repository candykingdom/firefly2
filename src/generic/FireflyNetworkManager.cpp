#include "FireflyNetworkManager.hpp"

#include <Debug.hpp>

FireflyNetworkManager::FireflyNetworkManager(Radio *const radio) : radio_(radio) {
  recent_ids_cache_.fill(0);
  recent_ids_cache_index_ = 0;
}

bool FireflyNetworkManager::receive(RadioPacket &packet) {
  if (!radio_->readPacket(packet)) {
    return false;
  }

  // Rebroadcast the packet if we haven't seen it's ID recently.
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    if (recent_ids_cache_[i] == packet.packet_id) {
      return false;
    }
  }

  radio_->sendPacket(packet);
  AddToRecentIdsCache(packet.packet_id);

  return true;
}

void FireflyNetworkManager::send(RadioPacket &packet) {
  // [2, 0xFFFF) allow us to use packet ID 1 in tests, so that the code under
  // test always wins master election.
  packet.packet_id = random(2, 0xFFFF);
  radio_->sendPacket(packet);
  AddToRecentIdsCache(packet.packet_id);
}

void FireflyNetworkManager::AddToRecentIdsCache(uint16_t id) {
  recent_ids_cache_[recent_ids_cache_index_++] = id;
  recent_ids_cache_index_ %= kRecentIdsCacheSize;
}
