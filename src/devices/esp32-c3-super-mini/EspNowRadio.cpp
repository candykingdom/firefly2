#include "EspNowRadio.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define ESP_NOW_WIFI_CHANNEL 4
#define ESPNOW_WIFI_IFACE WIFI_IF_STA

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Max RadioPacket wire encoding: 3-byte header + 58-byte payload.
static constexpr size_t kMaxWireLength =
    PACKET_HEADER_LENGTH + PACKET_DATA_LENGTH;

// A received ESP-NOW frame carried as the architecture-neutral RadioPacket wire
// encoding (see RadioPacket::Serialize), decoded by readPacket. Using the wire
// format keeps ESP-NOW consistent with the RFM69 and serial transports.
struct WireFrame {
  uint8_t bytes[kMaxWireLength];
  uint8_t len;
};

// Depth-1 mailbox between the ESP-NOW receive callback (WiFi task) and
// readPacket (loop task). xQueueOverwrite/xQueueReceive copy the frame under
// a critical section, so the reader never sees a torn frame; newest frame
// wins, matching the single-packet semantics of the RFM69 path.
QueueHandle_t rx_queue = nullptr;

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
    if (len == 0 || len > kMaxWireLength) {
      return;
    }
    WireFrame frame;
    memcpy(frame.bytes, data, len);
    frame.len = static_cast<uint8_t>(len);
    xQueueOverwrite(rx_queue, &frame);
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
  rx_queue = xQueueCreate(1, sizeof(WireFrame));
  if (rx_queue == nullptr) {
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESP_NOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }
  if (!ESP_NOW.begin((const uint8_t*)ESPNOW_PMK)) {
    return false;
  }
  ESP_NOW.onNewPeer(register_peer, this);
  return broadcast_peer.begin();
}

bool EspNowRadio::readPacket(RadioPacket& packet) {
  if (rx_queue == nullptr) {
    return false;
  }
  WireFrame frame;
  if (xQueueReceive(rx_queue, &frame, 0) != pdTRUE) {
    return false;
  }
  return packet.Deserialize(frame.bytes, frame.len);
}

void EspNowRadio::sendPacket(RadioPacket& packet) {
  uint8_t buffer[kMaxWireLength];
  const uint8_t len = packet.Serialize(buffer);
  // void returning function, don't check error status
  broadcast_peer.send_message(buffer, len);
}

void EspNowRadio::sleep() {}