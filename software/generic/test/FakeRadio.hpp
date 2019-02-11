#ifndef __FAKE_RADIO_H__
#define __FAKE_RADIO_H__

#include "../Radio.hpp"

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
  RadioPacket *receivedPacket;
  RadioPacket *sentPacket;
};

#endif
