#include "NetworkManager.hpp"

#include <Debug.hpp>

NetworkManager::NetworkManager(Radio *const radio) : radio_(radio) {
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    recent_ids_cache_[i] = 0;
    recent_ids_num_seen_[i] = 0;
  }
  recent_ids_cache_index_ = 0;
}

bool NetworkManager::receive(RadioPacket &packet) {
  if (!radio_->readPacket(packet)) {
    return false;
  }

  // Rebroadcast the packet if we haven't seen it's ID recently.
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    if (recent_ids_cache_[i] == packet.packet_id) {
      if (recent_ids_num_seen_[i] != 255) {
        recent_ids_num_seen_[i]++;
      }
      return false;
    }
  }

  AddToRecentIdsCache(packet.packet_id);
  if (resume_rebroadcast_at == 0 || resume_rebroadcast_at < millis()) {
    radio_->sendPacket(packet);
  }

  return true;
}

void NetworkManager::send(RadioPacket &packet) {
  // [2, 0xFFFF) allow us to use packet ID 1 in tests, so that the code under
  // test always wins master election.
  packet.packet_id = random(2, 0xFFFF);
  radio_->sendPacket(packet);
  AddToRecentIdsCache(packet.packet_id);
}

void NetworkManager::AddToRecentIdsCache(uint16_t id) {
  uint16_t sum = 0;
  for (uint8_t n = 0; n < kRecentIdsCacheSize; ++n) {
    sum += recent_ids_num_seen_[n];
  }
  if (sum > kPacketCopiesThreshold) {
    resume_rebroadcast_at = millis() + kRebroadcastPauseMillis;
  }

  recent_ids_cache_[recent_ids_cache_index_] = id;
  recent_ids_num_seen_[recent_ids_cache_index_] = 0;
  recent_ids_cache_index_ = (recent_ids_cache_index_ + 1) % kRecentIdsCacheSize;
}
