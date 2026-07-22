#include "SerialRadio.hpp"

// The serial bridge is opt-in: devices build it in with -DSERIAL_BRIDGE=1 (see
// platformio.ini), and only when the peer board is actually wired to the UART
// pins. With the bridge compiled in but nothing attached, the floating RX line
// feeds line noise into the mesh, and a garbage frame that happens to decode as
// a HEARTBEAT re-bases the network clock. The CRC in readPacket rejects almost
// all such noise, but the hazard is the reason to keep the flag off by default.

#ifdef SERIAL_RADIO_DEBUG
// Diagnostic logging to the USB CDC `Serial` port. Enabled only when built with
// -DSERIAL_RADIO_DEBUG (see the esp32-c3-super-mini env). Logs raw inbound
// bytes plus every decoded/sent packet, so you can tell "nothing on the wire"
// apart from "bytes arriving but framing/baud wrong". The owning device must
// have called Serial.begin() first.
namespace {
void DebugLogPacket(const char* dir, const RadioPacket& packet) {
  uint8_t n = packet.dataLength <= PACKET_DATA_LENGTH ? packet.dataLength
                                                      : PACKET_DATA_LENGTH;
  Serial.printf("[SerialRadio %s] id=%u type=%u len=%u data=", dir,
                packet.packet_id, static_cast<unsigned>(packet.type),
                packet.dataLength);
  for (uint8_t i = 0; i < n; i++) {
    Serial.printf("%02X ", packet.data[i]);
  }
  Serial.println();
}
}  // namespace
#endif

SerialRadio::SerialRadio(Stream& serial) : Radio(), serial_(serial) {}

bool SerialRadio::Begin() {
  // The owning device is responsible for calling serial_.begin() with the
  // correct baud/pins before this point. Discard any partial bytes so the
  // first real frame parses cleanly.
  rx_len_ = 0;
  return true;
}

bool SerialRadio::readPacket(RadioPacket& packet) {
  while (serial_.available() > 0) {
    uint8_t byte = static_cast<uint8_t>(serial_.read());
#ifdef SERIAL_RADIO_DEBUG
    // Echo every raw inbound byte so a stuck/silent node link is obvious.
    Serial.printf("%02X ", byte);
#endif

    if (byte == 0x00) {
      // End of frame. Attempt to decode whatever we've accumulated.
      size_t frame_len = rx_len_;
      rx_len_ = 0;
      if (frame_len == 0) {
        continue;  // Empty frame (e.g. a leading delimiter); skip.
      }
      // Sized to the frame, not the body: CobsDecode can emit up to
      // frame_len - 1 bytes before the length check below rejects it.
      uint8_t body[kMaxFrameLength];
      size_t body_len = serial_codec::CobsDecode(rx_buffer_, frame_len, body);
      // A frame must carry a CRC, and match it. Without this, COBS decodes
      // line noise from a floating RX pin into plausible packets.
      if (body_len > kCrcLength && body_len <= kMaxBodyLength &&
          serial_codec::Crc16(body, body_len) == 0 &&
          packet.Deserialize(body,
                             static_cast<uint8_t>(body_len - kCrcLength))) {
#ifdef SERIAL_RADIO_DEBUG
        Serial.println();
        DebugLogPacket("RX", packet);
#endif
        return true;
      }
#ifdef SERIAL_RADIO_DEBUG
      Serial.printf("\n[SerialRadio RX] malformed frame (%u bytes)\n",
                    static_cast<unsigned>(frame_len));
#endif
      // Malformed frame: drop it and keep draining.
      continue;
    }

    if (rx_len_ < sizeof(rx_buffer_)) {
      rx_buffer_[rx_len_++] = byte;
    } else {
      // Overflow without a delimiter: resync by dropping the frame.
      rx_len_ = 0;
    }
  }
  return false;
}

void SerialRadio::sendPacket(RadioPacket& packet) {
  uint8_t body[kMaxBodyLength];
  const uint8_t wire_len = packet.Serialize(body);
  const uint16_t crc = serial_codec::Crc16(body, wire_len);
  body[wire_len] = crc >> 8;
  body[wire_len + 1] = crc & 0xFF;

  uint8_t frame[kMaxFrameLength];
  size_t frame_len =
      serial_codec::CobsEncode(body, wire_len + kCrcLength, frame);

  serial_.write(frame, frame_len);
  serial_.write(static_cast<uint8_t>(0x00));  // Frame delimiter.
#ifdef SERIAL_RADIO_DEBUG
  DebugLogPacket("TX", packet);
#endif
}

void SerialRadio::sleep() {}

int16_t SerialRadio::LastRssi() { return 0; }
