#ifndef __FAKE_RADIO_H__
#define __FAKE_RADIO_H__

#include <Radio.hpp>

class FakeRadio : public Radio {
 public:
  FakeRadio();
  ~FakeRadio();

  // Overrides
  bool readPacket(RadioPacket &packet) override;
  void sendPacket(RadioPacket &packet) override;
  void sleep() {}

  // Test methods

  // Injects a packet for readPacket to return. Deliberately bypasses the wire
  // codec so tests can inject invalid packets the codec could never produce.
  void setReceivedPacket(RadioPacket *packet);

  // Returns the last packet passed to sendPacket after a round-trip through
  // the production wire codec (Serialize -> bytes -> Deserialize), so a
  // serialization defect is visible to every test that observes sent traffic.
  // Returns nullptr if nothing was sent or the codec rejects the frame.
  // The caller owns the returned packet.
  RadioPacket *getSentPacket();

 private:
  RadioPacket *received_packet;
  uint8_t sent_wire[PACKET_HEADER_LENGTH + PACKET_DATA_LENGTH];
  uint8_t sent_wire_length;
  bool has_sent_packet;
};

#endif
