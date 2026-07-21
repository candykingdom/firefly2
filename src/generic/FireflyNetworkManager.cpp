#include "FireflyNetworkManager.hpp"

#include <Debug.hpp>

FireflyNetworkManager::FireflyNetworkManager(Radio* const radio) : radios() {
  radios.emplace_back(radio);
}

void FireflyNetworkManager::rebroadcastPacket(RadioPacket& packet) {
  for (auto& radio : radios) {
    if (!radio.IsPacketInCache(packet.packet_id)) {
      radio.AddToRecentIdsCache(packet.packet_id);
      radio.radio->sendPacket(packet);
    }
  }
}

bool FireflyNetworkManager::receive(RadioPacket& packet) {
  const size_t radio_count = radios.size();
  for (size_t i = 0; i < radio_count; i++) {
    // Start the scan at the round-robin cursor rather than always at radios[0],
    // so a consistently busy radio can't starve the others behind it.
    const size_t idx = (next_radio_index_ + i) % radio_count;
    auto& radio_ = radios[idx];
    if (radio_.radio->readPacket(packet)) {
      // Advance the cursor past this radio so the next call gives a different
      // radio first look, even when we early-return on a duplicate below.
      next_radio_index_ = (idx + 1) % radio_count;
      // Transports only guarantee framing. Drop malformed packets before they
      // are cached, dispatched, or flooded onward.
      if (!packet.IsValid()) {
        return false;
      }
      // The mesh floods duplicates; deliver each recently-seen packet id to
      // the caller only once.
      if (radio_.IsPacketInCache(packet.packet_id)) {
        return false;
      }
      // Seed the source radio's cache so the rebroadcast below skips it via the
      // normal dedup path, while still forwarding onto every other radio. See
      // Radio::RebroadcastsToSource.
      if (!radio_.radio->RebroadcastsToSource()) {
        radio_.AddToRecentIdsCache(packet.packet_id);
      }
      rebroadcastPacket(packet);
      return true;
    }
  }
  return false;
}

void FireflyNetworkManager::send(RadioPacket& packet) {
  // [2, 0xFFFF) allow us to use packet ID 1 in tests, so that the code under
  // test always wins master election.
  packet.packet_id = random(2, 0xFFFF);
  for (auto& radio : radios) {
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
  recent_ids_cache_index = (recent_ids_cache_index + 1) % kRecentIdsCacheSize;
}

bool RadioWrapper::IsPacketInCache(const uint16_t id) {
  for (uint8_t i = 0; i < kRecentIdsCacheSize; i++) {
    if (recent_ids_cache_[i] == id) {
      return true;
    }
  }
  return false;
}
