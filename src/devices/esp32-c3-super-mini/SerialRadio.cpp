#include "SerialRadio.hpp"

void print_packet(char* operation, RadioPacket& packet) {
  static char buffer[64];
  int len =
      snprintf(buffer, sizeof(buffer), "%s id: %d, type %d, len %d", operation,
               packet.packet_id, packet.type, packet.dataLength);
  if (Serial.availableForWrite() >= len) {
    Serial.println(buffer);
  }
}

void transmit_packet(RadioPacket& packet) {
  // 2 bytes for packet_id, 1 for type, 1 for dataLength, n for data
  if (Serial1.availableForWrite() >= 4 + packet.dataLength) {
    Serial1.write((uint8_t*)&packet.packet_id, sizeof(packet.packet_id));
    Serial1.write(packet.type);
    Serial1.write(packet.dataLength);
    Serial1.write((uint8_t*)&packet.data,
                  min(packet.dataLength, (uint8_t)packet.data.size()));
  }
}

SerialRadio::SerialRadio() : Radio() {}

bool SerialRadio::Begin() {
  Serial1.begin(115200, SERIAL_8N1, 1, 2);
  // Proper framing using an encoding like Consistent Overhead Byte Stuffing
  // should eliminate the need to wait for the serial connection to stabilize
  // before reading data from it.
  delay(5000);
  return true;
}

bool SerialRadio::readPacket(RadioPacket& packet) {
  if (Serial1.available() >= 4) {
    packet.packet_id = Serial1.read() & 0xFF;
    packet.packet_id |= packet.packet_id + (Serial1.read() << 8);
    packet.type = (PacketType)Serial1.read();
    packet.dataLength = Serial1.read();
    Serial1.readBytes((char*)packet.data.data(),
                      min(packet.dataLength, (uint8_t)packet.data.size()));
    return true;
  }
  return false;
}

void SerialRadio::sendPacket(RadioPacket& packet) { transmit_packet(packet); }

void SerialRadio::sleep() {}

int16_t SerialRadio::LastRssi() { return 0; }