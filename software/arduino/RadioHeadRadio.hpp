#ifndef __RADIO_HEAD_RADIO_H__
#define __RADIO_HEAD_RADIO_H__

#include <RH_RF69.h>
#include "Radio.hpp"

const int kMaxFifoSizePacketSize = 64;
const int kRadioOverhead = 3;
const int kMaxPacketSize = kMaxFifoSizePacketSize - kRadioOverhead;
const int kFrontPacketPadding = 3;

class RadioHeadRadio : public Radio {
 public:
  RadioHeadRadio();

  // Overrides
  bool readPacket(RadioPacket &packet) override;
  void sendPacket(RadioPacket &packet) override;

 private:
  RH_RF69 radio = RH_RF69(4, 5);
};

#endif
