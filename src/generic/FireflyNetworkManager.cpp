#include "FireflyNetworkManager.hpp"

#include <Debug.hpp>

FireflyNetworkManager::FireflyNetworkManager(Radio* const radio) : radios() {
  radios.emplace_back(radio);
}

void FireflyNetworkManager::rebroadcastPacket(RadioPacket& packet) {
  for (auto radio : radios) {
    if (!radio.IsPacketInCache(packet.packet_id)) {
      radio.AddToRecentIdsCache(packet.packet_id);
      radio.radio->sendPacket(packet);
    }
  }
}

bool FireflyNetworkManager::receive(RadioPacket& packet) {
  for (auto radio_ : radios) {
    if (radio_.radio->readPacket(packet)) {
      rebroadcastPacket(packet);
      // Unfair, early return can starve a busy radio behind a less busy one
      return true;
    }
  }
  return false;
}

void FireflyNetworkManager::send(RadioPacket& packet) {
  // [2, 0xFFFF) allow us to use packet ID 1 in tests, so that the code under
  // test always wins master election.
  packet.packet_id = random(2, 0xFFFF);
  for (auto radio : radios) {
    radio.radio->sendPacket(packet);
    radio.AddToRecentIdsCache(packet.packet_id);
  }
}

void FireflyNetworkManager::addRadio(Radio* const radio) {
  radios.emplace_back(radio);
}

RadioWrapper::RadioWrapper(Radio* const radio) : radio(radio) {
  recent_ids_cache_.fill(0);
  recent_ids_cache_index = 0;
}

void RadioWrapper::AddToRecentIdsCache(const uint16_t id) {
  recent_ids_cache_[recent_ids_cache_index] = id;
  recent_ids_cache_index %= kRecentIdsCacheSize;
}

bool RadioWrapper::IsPacketInCache(const uint16_t id) {
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    if (recent_ids_cache_[i] == id) {
      return true;
    }
  }
  return false;
}
