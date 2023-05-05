#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <Radio.hpp>
#include <Types.hpp>
#include <array>

class NetworkManager {
 public:
  explicit NetworkManager(Radio *const radio);

  /**
   * Checks if a packet is available. If so, performs network functions (e.g.
   * rebroadcasting), copies the packet into the passed-in struct, and
   * returns true. If no packet is available, returns false.
   */
  bool receive(RadioPacket &packet);

  /**
   * Sends the given packet.
   */
  void send(RadioPacket &packet);

  // Public for testing
  static const uint8_t kRecentIdsCacheSize = 5;

 private:
  void AddToRecentIdsCache(uint16_t id);

  Radio *const radio_;

  /**
   * We maintain a circular buffer of the most recent packet IDs seen. When a
   * packet is sent or received, its packet ID is added to this buffer, and the
   * LRU ID is dropped.
   */
  std::array<uint16_t, kRecentIdsCacheSize> recent_ids_cache_;
  uint8_t recent_ids_cache_index_;
};

#endif
