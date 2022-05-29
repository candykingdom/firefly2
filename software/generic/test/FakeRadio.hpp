#ifndef __FAKE_RADIO_H__
#define __FAKE_RADIO_H__

#include "../../lib/radio/Radio.hpp"

class FakeRadio : public Radio {
 public:
  FakeRadio();

  // Overrides
  bool readPacket(RadioPacket &packet) override;
  void sendPacket(RadioPacket &packet) override;
  void sleep() {}

  // Test methods
  void setReceivedPacket(RadioPacket *packet);
  RadioPacket *getSentPacket();

 private:
  RadioPacket *received_packet;
  RadioPacket *sent_packet;
};

#endif
