#ifndef ESP_NOW_RADIO_H
#define ESP_NOW_RADIO_H

#include <ESP32_NOW.h>
#include <WiFi.h>

#include <Radio.hpp>

#define ESPNOW_PMK "fireflysamplepmk"
#define ESPNOW_LMK "fireflysamplelmk"

class EspNowRadio : public Radio {
 public:
  EspNowRadio();

  bool Begin();

  bool readPacket(RadioPacket& packet) override;
  void sendPacket(RadioPacket& packet) override;
  void saveData(uint8_t* data, size_t len);
  void sleep() override;

  int16_t LastRssi();

 private:
  uint16_t last_packet_id = 0;
};

#endif  // ESP_NOW_RADIO_H