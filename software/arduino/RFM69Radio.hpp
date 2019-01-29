#ifndef __RFM69_RADIO_H__
#define __RFM69_RADIO_H__

#include <RFM69.h>
#include "Radio.hpp"

const int kMaxFifoSizePacketSize = 64;
const int kRadioOverhead = 3;
const int kMaxPacketSize = kMaxFifoSizePacketSize - kRadioOverhead;
const int kFrontPacketPadding = 3;

const int kNodeId = 57;
const int kNetworkId = 137;

class RFM69Radio : public Radio {
 public:
  RFM69Radio();

  // Overrides
  bool readPacket(RadioPacket &packet) override;
  void sendPacket(RadioPacket &packet) override;

 private:
  RFM69 *radio;
};

#endif
