#ifndef __SERIAL_RADIO_HPP__
#define __SERIAL_RADIO_HPP__

#include <Radio.hpp>

class SerialRadio : public Radio {
 public:
  SerialRadio();

  bool Begin();

  bool readPacket(RadioPacket& packet) override;
  void sendPacket(RadioPacket& packet) override;
  void saveData(uint8_t* data, size_t len);
  void sleep() override;

  int16_t LastRssi();

 private:
  char buffer[sizeof(RadioPacket) * 2];
  size_t buffer_index = 0;
};

#endif  // __SERIAL_RADIO_HPP__