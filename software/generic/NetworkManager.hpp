#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include "Radio.hpp"
#include "Types.hpp"

class NetworkManager {
 public:
  NetworkManager(Radio *const radio);

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

 private:
  Radio *const radio;
};

#endif
