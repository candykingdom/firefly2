#include "SerialCodec.hpp"

namespace serial_codec {

size_t CobsEncode(const uint8_t *in, size_t len, uint8_t *out) {
  size_t read_index = 0;
  size_t write_index = 1;
  size_t code_index = 0;
  uint8_t code = 1;

  while (read_index < len) {
    if (in[read_index] == 0) {
      out[code_index] = code;
      code = 1;
      code_index = write_index++;
      read_index++;
    } else {
      out[write_index++] = in[read_index++];
      code++;
      if (code == 0xFF) {
        out[code_index] = code;
        code = 1;
        code_index = write_index++;
      }
    }
  }
  out[code_index] = code;
  return write_index;
}

size_t CobsDecode(const uint8_t *in, size_t len, uint8_t *out) {
  size_t read_index = 0;
  size_t write_index = 0;

  while (read_index < len) {
    uint8_t code = in[read_index];
    if (code == 0 || read_index + code > len + 1) {
      return 0;  // Malformed frame.
    }
    read_index++;
    for (uint8_t i = 1; i < code && read_index < len; i++) {
      out[write_index++] = in[read_index++];
    }
    if (code != 0xFF && read_index < len) {
      out[write_index++] = 0;
    }
  }
  return write_index;
}

uint16_t Crc16(const uint8_t *in, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)in[i] << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                           : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

}  // namespace serial_codec
