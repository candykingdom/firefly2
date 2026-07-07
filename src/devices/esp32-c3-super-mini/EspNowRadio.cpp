#include "EspNowRadio.hpp"

#define ESP_NOW_WIFI_CHANNEL 4
#define ESPNOW_WIFI_IFACE WIFI_IF_STA
#define ESPNOW_EXAMPLE_PMK "pmk1234567890123"
#define ESPNOW_EXAMPLE_LMK "lmk1234567890123"

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t saved_data[ESP_NOW_MAX_DATA_LEN_V2];
size_t saved_data_len;

class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
 public:
  // Constructor of the class using the broadcast address
  ESP_NOW_Broadcast_Peer(const uint8_t* mac_addr, const uint8_t* lmk)
      : ESP_NOW_Peer(mac_addr, ESP_NOW_WIFI_CHANNEL, ESPNOW_WIFI_IFACE, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Broadcast_Peer() { remove(); }

  // Function to properly initialize the ESP-NOW and register the broadcast peer
  bool begin() {
    if (!add()) {
      log_e("Failed to initialize ESP-NOW or register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to send a message to all devices within the network
  bool send_message(const uint8_t* data, size_t len) {
    return send(data, len) > 0;
  }
  void onReceive(const uint8_t* data, size_t len, bool broadcast) {
    if (len == sizeof(RadioPacket)) {
      saved_data_len = len;
      memcpy(saved_data, data, saved_data_len);
    }
  }
};

ESP_NOW_Broadcast_Peer broadcast_peer(broadcastAddress, nullptr);

void register_peer(const esp_now_recv_info_t* info, const uint8_t* data,
                   int len, void* arg) {
  ESP_NOW_Broadcast_Peer* new_peer =
      new ESP_NOW_Broadcast_Peer(info->src_addr, nullptr);
  if (!new_peer->begin()) {
    delete new_peer;
  }
}

EspNowRadio::EspNowRadio() : Radio() {}

bool EspNowRadio::Begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESP_NOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }
  if (!ESP_NOW.begin((const uint8_t*)ESPNOW_EXAMPLE_PMK)) {
    return false;
  }
  ESP_NOW.onNewPeer(register_peer, this);
  return broadcast_peer.begin();
}

bool EspNowRadio::readPacket(RadioPacket& packet) {
  memcpy(&packet, saved_data, saved_data_len);
  return true;
}

void EspNowRadio::sendPacket(RadioPacket& packet) {
  // void returning function, don't check error status
  broadcast_peer.send_message((uint8_t*)&packet, sizeof(packet));
}

void EspNowRadio::saveData(uint8_t* data, size_t len) {
  memcpy(data, saved_data, saved_data_len);
  saved_data_len = len;
}

void EspNowRadio::sleep() {}