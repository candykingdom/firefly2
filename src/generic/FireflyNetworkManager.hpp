#ifndef __FIREFLY_NETWORK_MANAGER_H__
#define __FIREFLY_NETWORK_MANAGER_H__

#include <Radio.hpp>
#include <Types.hpp>
#include <array>
#include <vector>

class RadioWrapper {
 public:
  RadioWrapper(Radio* const radio);
  void AddToRecentIdsCache(const uint16_t id);
  bool IsPacketInCache(const uint16_t id);
  Radio* const radio;
  static const uint8_t kRecentIdsCacheSize = 5;

 private:
  /**
   * We maintain a per-radio circular buffer of the most recent packet IDs seen.
   * When a packet is sent or received, its packet ID is added to this buffer,
   * and the LRU ID is dropped.
   */
  std::array<uint16_t, kRecentIdsCacheSize> recent_ids_cache_;
  uint8_t recent_ids_cache_index;
};

class FireflyNetworkManager {
 public:
  void AddToRecentIdsCache(uint16_t id);

  explicit FireflyNetworkManager(Radio* const radio);

  /**
   * Checks if a packet is available. If so, performs network functions (e.g.
   * rebroadcasting), copies the packet into the passed-in struct, and
   * returns true. If no packet is available, returns false.
   */
  bool receive(RadioPacket& packet);

  /**
   * Rebroadcast a packet to every radio whose recent-id cache doesn't already
   * hold this packet's id. To stop a packet being echoed back out the radio it
   * arrived on, the caller seeds that radio's cache first (see receive()).
   */
  void rebroadcastPacket(RadioPacket& packet);

  /**
   * Add an additional radio to manage
   */
  void addRadio(Radio* const radio);

  /**
   * Sends the given packet.
   */
  void send(RadioPacket& packet);

 private:
  std::vector<RadioWrapper> radios;

  /**
   * Round-robin cursor for receive(). Each call starts its scan here and, on
   * finding a packet, advances past that radio so a consistently busy radio
   * can't monopolize the single read per tick and starve the others. An index
   * (rather than an iterator) survives the vector reallocation in addRadio().
   */
  size_t next_radio_index_ = 0;
};

#endif  // __FIREFLY_NETWORK_MANAGER_H__
